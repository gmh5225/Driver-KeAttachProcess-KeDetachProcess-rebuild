void eprocess_t::attach( )
{
	 // KeAttachProcess stuff
	 kprcb_t* prcb = utils::get_cur_prcb( );
	 ethread_t* cur_thread = utils::get_cur_thread( );
	 KAPC_STATE* cur_apc_state = &cur_thread->m_apc_state( );
	 KAPC_STATE* saved_apc_state = &cur_thread->m_saved_apc_state( );
 
	 uint64_t old_cr8 = __readcr8( );
	 __writecr8( 2 );
 
	 if ( *g_dynamic_data.m_ki_irql_flags && *g_dynamic_data.m_ki_irql_flags & 1 )
		  prcb->m_scheduler_assist( )[ 5 ] |= ( uint32_t )( ( 0xFFFFFFFFFFFFFFFF << ( ( old_cr8 & 0xFF ) + 1 ) ) & 4 );
 
	 while ( 1 )
	 {
		  if ( prcb->m_scheduler_assist( ) && prcb->m_nesting_level( ) <= 1 && prcb->m_scheduler_assist( )[ 6 ]++ == -1 )
				safe_calls::call( g_dynamic_data.m_ki_remove_system_work_priority_kick, prcb );
 
		  if ( !_interlockedbittestandset64( reinterpret_cast< volatile LONG64* >( &cur_thread->m_thread_lock( ) ), 0 ) )
				break;
 
		  if ( prcb->m_scheduler_assist( ) && prcb->m_nesting_level( ) <= 1 && !( --prcb->m_scheduler_assist( )[ 6 ] ) )
				safe_calls::call( g_dynamic_data.m_ki_remove_system_work_priority_kick, prcb );
 
		  uint64_t val = 0;
 
		  do
				safe_calls::call( g_dynamic_data.m_ke_yield_processor, &val );
		  while ( cur_thread->m_thread_lock( ) );
	 }
 
	 // KiAttachProcess stuff
	 saved_apc_state->Process = cur_apc_state->Process;
	 saved_apc_state->InProgressFlags = cur_apc_state->InProgressFlags;
	 saved_apc_state->KernelApcPending = cur_apc_state->KernelApcPending;
	 saved_apc_state->UserApcPendingAll = cur_apc_state->UserApcPendingAll;
 
	 if ( cur_apc_state->ApcListHead[ 0 ].Flink == reinterpret_cast< LIST_ENTRY* >( cur_apc_state ) )
	 {
		  saved_apc_state->ApcListHead[ 0 ].Blink = reinterpret_cast< LIST_ENTRY* >( saved_apc_state );
		  saved_apc_state->ApcListHead[ 0 ].Flink = reinterpret_cast< LIST_ENTRY* >( saved_apc_state );
 
		  saved_apc_state->KernelApcPending = 0;
	 }
	 else
	 {
		  LIST_ENTRY* blink = cur_apc_state->ApcListHead[ 0 ].Blink;
		  LIST_ENTRY* flink = cur_apc_state->ApcListHead[ 0 ].Flink;
		  saved_apc_state->ApcListHead[ 0 ].Flink = flink;
		  saved_apc_state->ApcListHead[ 0 ].Blink = blink;
		  flink->Blink = reinterpret_cast< LIST_ENTRY* >( saved_apc_state );
		  blink->Flink = reinterpret_cast< LIST_ENTRY* >( saved_apc_state );
	 }
 
	 if ( cur_apc_state->ApcListHead[ 1 ].Flink == &cur_apc_state->ApcListHead[ 1 ] )
	 {
		  saved_apc_state->ApcListHead[ 1 ].Blink = &saved_apc_state->ApcListHead[ 1 ];
		  saved_apc_state->ApcListHead[ 1 ].Flink = &saved_apc_state->ApcListHead[ 1 ];
		  saved_apc_state->UserApcPendingAll = 0;
	 }
	 else
	 {
		  saved_apc_state->ApcListHead[ 1 ].Flink = cur_apc_state->ApcListHead[ 1 ].Flink;
		  saved_apc_state->ApcListHead[ 1 ].Blink = cur_apc_state->ApcListHead[ 1 ].Blink;
		  cur_apc_state->ApcListHead[ 1 ].Flink->Blink = &saved_apc_state->ApcListHead[ 1 ];
		  cur_apc_state->ApcListHead[ 1 ].Blink->Flink = &saved_apc_state->ApcListHead[ 1 ];
	 }
 
	 cur_apc_state->ApcListHead[ 0 ].Blink = reinterpret_cast< LIST_ENTRY* >( cur_apc_state );
 
	 cur_apc_state->ApcListHead[ 1 ].Blink = &cur_apc_state->ApcListHead[ 1 ];
	 cur_apc_state->ApcListHead[ 1 ].Flink = &cur_apc_state->ApcListHead[ 1 ];
	 cur_apc_state->ApcListHead[ 0 ].Flink = reinterpret_cast< LIST_ENTRY* >( cur_apc_state );
	 cur_apc_state->InProgressFlags = 0;
	 cur_apc_state->KernelApcPending = 0;
	 cur_apc_state->UserApcPendingAll = 0;
	 
	 cur_thread->m_apc_state_index( ) = 1;
 
	 if ( _InterlockedAnd( reinterpret_cast< volatile LONG* >( &m_stack_count( ) ), -1 ) & 7 )
		  drv_log( "[-] attach: pasta needed\n" );
 
	 cur_thread->m_misc_flags( ).m_process_detach_active = 1;
 
	 cur_thread->m_proc( ) = this;
	 
	 cur_thread->m_thread_lock( ) = 0;
	 if ( prcb->m_scheduler_assist( ) && prcb->m_nesting_level( ) <= 1 && !( --prcb->m_scheduler_assist( )[ 6 ] ) )
		  safe_calls::call( g_dynamic_data.m_ki_remove_system_work_priority_kick, prcb );
 
	 _interlockedbittestandset64( reinterpret_cast< volatile LONG64* >( &m_active_processors( ).m_bitmap[ prcb->m_group( ) ] ), prcb->m_group_index( ) );
 
	 if ( *g_dynamic_data.m_ki_kva_shadow )
	 {
		  uint64_t gs_value = m_directory_table_base( );
		  if ( gs_value & 2 )
				gs_value |= 0x8000000000000000;
		  __writegsqword( 0x9000, gs_value );
 
		  safe_calls::call( g_dynamic_data.m_ki_set_address_policy, m_address_policy( ) );
	 }
 
	 __writecr3( m_directory_table_base( ) );
 
	 if ( !*g_dynamic_data.m_ki_flush_pcid && *g_dynamic_data.m_ki_kva_shadow )
	 {
		  volatile uint64_t cr4 = __readcr4( );
		  if ( cr4 & 0x20080 )
		  {
				__writecr4( cr4 ^ 0x80 );
				__writecr4( cr4 );
		  }
		  else
		  {
				volatile uint64_t cr3 = __readcr3( );
				__writecr3( cr3 );
		  }
	 }
 
	 _interlockedbittestandreset64( reinterpret_cast< volatile LONG64* >( &m_active_processors( ).m_bitmap[ prcb->m_group( ) ] ), prcb->m_group_index( ) );
 
	 cur_thread->m_misc_flags( ).m_process_detach_active = 0;
 
	 if ( *g_dynamic_data.m_ki_irql_flags & 1 )
	 {
		  uint64_t scheduler_assist_val = ~( 0xFFFFFFFFFFFFFFFF << ( old_cr8 + 1 ) );
		  bool eq = ( scheduler_assist_val & prcb->m_scheduler_assist( )[ 5 ] ) == 0;
		  prcb->m_scheduler_assist( )[ 5 ] &= scheduler_assist_val;
 
		  if ( eq )
				safe_calls::call( g_dynamic_data.m_ki_remove_system_work_priority_kick, prcb );
	 }
 
	 __writecr8( old_cr8 );
}
