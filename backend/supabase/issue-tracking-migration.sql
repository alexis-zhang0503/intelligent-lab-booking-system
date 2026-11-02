-- Adds the repair outcome needed for a complete device maintenance trail.
-- Run once in the Supabase SQL Editor for existing projects.

alter table public.issue_reports
  add column if not exists resolution_note text;

alter table public.issue_reports
  drop constraint if exists issue_reports_resolution_note_length;

alter table public.issue_reports
  add constraint issue_reports_resolution_note_length
  check (resolution_note is null or char_length(trim(resolution_note)) between 1 and 120);
