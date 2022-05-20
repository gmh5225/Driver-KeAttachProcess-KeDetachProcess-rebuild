void eprocess_t::detach( )
{
	 ethread_t* cur_thread = utils::get_cur_thread( );
	 kprcb_t* prcb = utils::get_cur_prcb( );
	 KAPC_STATE* cur_apc_state = &cur_thread->m_apc_state( );
	 KAPC_STATE* saved_apc_state = &cur_thread->m_saved_apc_state( );
 
	 uint64_t old_cr8 = __readcr8( );
	 __writecr8( 2 );
 
	 if ( *g_dynamic_data.m_ki_irql_flags && *g_dynamic_data.m_ki_irql_flags & 1 )
		  prcb->m_scheduler_assist( )[ 5 ] |= ( uint32_t )( ( 0xFFFFFFFFFFFFFFFF << ( old_cr8 + 1 ) ) & 4 );
 
	 if ( prcb->m_scheduler_assist( ) && prcb->m_nesting_level( ) <= 1 && prcb->m_scheduler_assist( )[ 6 ]++ == -1 )
		  safe_calls::call( g_dynamic_data.m_ki_remove_system_work_priority_kick, prcb );
 
	 while ( _interlockedbittestandset64( reinterpret_cast< volatile LONG64* >( &cur_thread->m_thread_lock( ) ), 0 ) )
	 {
		  if ( prcb->m_scheduler_assist( ) && prcb->m_nesting_level( ) <= 1 && !( --prcb->m_scheduler_assist( )[ 6 ] ) )
				safe_calls::call( g_dynamic_data.m_ki_remove_system_work_priority_kick, prcb );
 
		  uint64_t val = 0;
		  do
				safe_calls::call( g_dynamic_data.m_ke_yield_processor, &val );
		  while ( cur_thread->m_thread_lock( ) );
 
		  if ( prcb->m_scheduler_assist( ) && prcb->m_nesting_level( ) <= 1 && ( prcb->m_scheduler_assist( )[ 6 ]++ ) == -1 )
				safe_calls::call( g_dynamic_data.m_ki_remove_system_work_priority_kick, prcb );
	 }
 
	 cur_thread->m_misc_flags( ).m_process_detach_active = 1;
 
	 cur_apc_state->Process = saved_apc_state->Process;
	 cur_apc_state->InProgressFlags = saved_apc_state->InProgressFlags;
	 cur_apc_state->KernelApcPending = saved_apc_state->KernelApcPending;
	 cur_apc_state->UserApcPendingAll = saved_apc_state->UserApcPendingAll;
	 LIST_ENTRY* Flink = saved_apc_state->ApcListHead[ 0 ].Flink;
 
	 if ( saved_apc_state->ApcListHead[ 0 ].Flink == reinterpret_cast< LIST_ENTRY* >( saved_apc_state ) )
	 {
		  cur_apc_state->ApcListHead[ 0 ].Blink = reinterpret_cast< LIST_ENTRY* >( cur_apc_state );
		  cur_apc_state->ApcListHead[ 0 ].Flink = reinterpret_cast< LIST_ENTRY* >( cur_apc_state );
		  cur_apc_state->KernelApcPending = 0;
	 }
	 else
	 {
		  LIST_ENTRY* Blink = saved_apc_state->ApcListHead[ 0 ].Blink;
		  cur_apc_state->ApcListHead[ 0 ].Flink = Flink;
		  cur_apc_state->ApcListHead[ 0 ].Blink = Blink;
		  Flink->Blink = reinterpret_cast< LIST_ENTRY* >( cur_apc_state );
		  Blink->Flink = reinterpret_cast< LIST_ENTRY* >( cur_apc_state );
	 }
 
	 LIST_ENTRY* saved_um_flink = saved_apc_state->ApcListHead[ 1 ].Flink;
	 LIST_ENTRY* cur_um_head = &cur_apc_state->ApcListHead[ 1 ];
 
	 if ( saved_um_flink == &saved_apc_state->ApcListHead[ 1 ] )
	 {
		  cur_apc_state->ApcListHead[ 1 ].Blink = &cur_apc_state->ApcListHead[ 1 ];
		  cur_um_head->Flink = cur_um_head;
		  cur_apc_state->UserApcPendingAll = 0;
	 }
	 else
	 {
		  LIST_ENTRY* saved_um_blink = saved_apc_state->ApcListHead[ 1 ].Blink;
		  cur_um_head->Flink = saved_um_flink;
		  cur_apc_state->ApcListHead[ 1 ].Blink = saved_um_blink;
		  saved_um_flink->Blink = cur_um_head;
		  saved_um_blink->Flink = cur_um_head;
	 }
 
	 saved_apc_state->Process = 0;
	 cur_thread->m_apc_state_index( ) = 0;
 
	 cur_thread->m_thread_lock( ) = 0;
	 if ( prcb->m_scheduler_assist( ) && prcb->m_nesting_level( ) <= 1 && !( --prcb->m_scheduler_assist( )[ 6 ] ) )
		  safe_calls::call( g_dynamic_data.m_ki_remove_system_work_priority_kick, prcb );
 
	 _interlockedbittestandset64( reinterpret_cast< volatile LONG64* >( &m_active_processors( ).m_bitmap[ prcb->m_group( ) ] ), prcb->m_group_index( ) );
 
	 if ( *g_dynamic_data.m_ki_kva_shadow )
	 {
		  uint64_t gs_value = cur_thread->m_proc( )->m_directory_table_base( );
		  if ( gs_value & 2 )
				gs_value |= 0x8000000000000000;
 
		  __writegsqword( 0x9000, gs_value );
 
		  safe_calls::call( g_dynamic_data.m_ki_set_address_policy, m_address_policy( ) );
	 }
 
	 __writecr3( cur_thread->m_proc( )->m_directory_table_base( ) );
 
	 if ( !*g_dynamic_data.m_ki_flush_pcid && *g_dynamic_data.m_ki_kva_shadow )
	 {
		  uint64_t cr4 = __readcr4( );
		  if ( cr4 & 0x20080 )
		  {
				__writecr4( cr4 ^ 0x80 );
				__writecr4( cr4 );
		  }
		  else
		  {
				uint64_t cr3 = __readcr3( );
				__writecr3( cr3 );
		  }
	 }
 
	 _interlockedbittestandreset64( reinterpret_cast< volatile LONG64* >( &m_active_processors( ).m_bitmap[ prcb->m_group( ) ] ), prcb->m_group_index( ) );
 
	 cur_thread->m_misc_flags( ).m_process_detach_active = 0;
 
	 if ( *g_dynamic_data.m_ki_irql_flags && *g_dynamic_data.m_ki_irql_flags & 1 )
	 {
		  uint64_t scheduler_assist_val = ~( 0xFFFFFFFFFFFFFFFF << ( old_cr8 + 1 ) );
		  bool eq = ( scheduler_assist_val & prcb->m_scheduler_assist( )[ 5 ] ) == 0;
		  prcb->m_scheduler_assist( )[ 5 ] &= scheduler_assist_val;
		  if ( eq )
				safe_calls::call( g_dynamic_data.m_ki_remove_system_work_priority_kick, prcb );
	 }
 
	 __writecr8( old_cr8 );
 
	 if ( ( m_stack_count( ) & 0xFFFFFFF8 ) == 8 )
		  drv_log( "[-] detach: pasta needed\n" );
 
	 if ( cur_apc_state->ApcListHead[ 0 ].Flink != reinterpret_cast< LIST_ENTRY* >( cur_apc_state ) )
	 {
		  cur_apc_state->KernelApcPending = 1;
		  safe_calls::import( utils::m_ntos, HASH( "HalRequestSoftwareInterrupt" ), 1ull );
	 }
}
