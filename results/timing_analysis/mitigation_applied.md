# h1 timing: mitigation decision

none applied, none needed in the r1/r2 threat model. reasoning:

- keycheck_verify is a local, offline operation. the caller invokes it
  with the key and generation it already has and reads the boolean
  result. a timing oracle leaks nothing the return value does not.
- the measured residual effect (median delta 8 ns of ~3000 ns, cohen's
  d -0.0019) is below the harness position bias of the same magnitude
  class (a/a calibration d 0.0199) - see statistical_test.md. the
  constant time compare (volatile accumulate, no branches on secret
  data) behaves as designed.
- random delay masking was considered and rejected: it consumes
  entropy per call, does not survive averaging by a patient network
  adversary, and would be security theater for an offline operation.

revisit trigger: the r4 directory service exposes generation
verification over the network. at that point verification moves into
the attacker-visible timing domain and needs network-level blinding
(uniform response padding or delayed batch response), decided in the
r4 threat model. requirement recorded in docs/r1_security.md follow
ups and docs/threat_model.md.
