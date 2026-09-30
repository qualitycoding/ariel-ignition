# Profiles
| Profile | Active | Reason |
|---|---|---|
| software | **yes** | firmware + host tests + simulation |
| software.deploys | **yes** | firmware is flashed into a vehicle's ignition; install and rollback runbook in OPERATIONS.md |
| math | no | no proofs or derivations are the deliverable (spike calculations only) |
| computational | no | no numerical experiments are the deliverable |
| publication | no | no paper |
Consequences: G-001 N/A; OPERATIONS.md required; security tests (CLI input) and
dependency scan (vendor hashes) required; no computational reproducibility gate.
