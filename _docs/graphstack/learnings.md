## 2026-09-02 — First mach1 graphstack loop: eight tasks shipped, PR #1 merged

- **Worked:** When scaffold ACs went stale against a saturator already in `processBlock`, supersede the ACs (and keep host gates on `host-validation`) instead of gutting DSP to match the old list. Designed `FAIL-UNVERIFIED` printers for missing DAWs kept Test honest.
- **Didn't work:** Landing the plugin on `main` then shipping from a ledger-only branch forced a SHA-alignment dance (`1c912f2` → re-record at `cd1be56`). `juce::jlimit` does not reject NaN; AG `dryRms/wetRms` is unbounded if wet RMS collapses. Both needed a Ship-time adversarial fix.
- **Watch for:** Ship's gate SHA must be an ancestor of HEAD — record Review/Test/evidence against the commit you will actually ship, not an earlier one. No CI on this repo. Host-validation in-app ACs stay UNVERIFIABLE until Logic/Reaper exist on the machine.
