import { createClient } from "../vendor/supabase.js";

/**
 * Public browser configuration for the shared lab database.
 * This publishable key identifies the project; RLS policies enforce access.
 * Never place a Supabase secret key in this file.
 */
export const supabase = createClient(
  "https://ikfqcappastopzjbncea.supabase.co",
  "sb_publishable_5Q5xIYHsAeEfO195kJ7jNg_Z5uBwxkV"
);

