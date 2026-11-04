-- V3.2 keeps the pre-existing optional repair-note contract while retaining V3 proposals.
-- Apply after V3.1. No reservation or issue record is changed by this migration.

create or replace function public.v3_create_fault_proposal(p_session_id uuid, p_action_type text, p_issue_id uuid, p_note text)
returns jsonb language plpgsql security definer set search_path=public
as $$
declare v_proposal_id uuid; v_note text := trim(coalesce(p_note, ''));
begin
  if not public.is_admin() then raise exception 'Administrator access is required'; end if;
  if p_action_type not in ('isolate_fault','complete_fault_repair') or p_issue_id is null then raise exception 'Invalid fault proposal'; end if;
  if (p_action_type = 'isolate_fault' and char_length(v_note) not between 1 and 200)
     or (p_action_type = 'complete_fault_repair' and char_length(v_note) > 120) then
    raise exception 'Invalid fault proposal note';
  end if;
  if not exists(select 1 from public.agent_sessions where id=p_session_id and user_id=auth.uid() and agent_type='fault_responder' and expires_at>now()) then raise exception 'The fault session is unavailable or expired'; end if;
  if not exists(select 1 from public.issue_reports where id=p_issue_id) then raise exception 'The issue was not found'; end if;
  insert into public.agent_proposals(user_id,session_id,agent_type,action_type,state,payload)
  values(auth.uid(),p_session_id,'fault_responder',p_action_type,'ready',jsonb_build_object('issue_id',p_issue_id,'note',v_note,'auto_reschedule',false)) returning id into v_proposal_id;
  return (select jsonb_build_object('proposalId',id,'state',state,'expiresAt',expires_at,'actionType',action_type) from public.agent_proposals where id=v_proposal_id);
end;
$$;

revoke all on function public.v3_create_fault_proposal(uuid,text,uuid,text) from public;
grant execute on function public.v3_create_fault_proposal(uuid,text,uuid,text) to authenticated;
