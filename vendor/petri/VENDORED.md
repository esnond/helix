# vendored: petri engine

verbatim import of the petri c. elegans connectome simulator, used here as the
key derivation core. no file modified. bitwise reproducibility of the kdf
inherits from the engine's own determinism (xorshift64* rng, seeded noise,
fixed fp operation order under -ffp-contract=off).

- source: ~/elegans (esnond/petri), release v1.0.1, commit 127ce2a
- import date: 2026-10-28
- flags: -std=c99 -pedantic -Wall -Wextra -Werror -O2 -ffp-contract=off
         -D_DEFAULT_SOURCE (same as engine release build)
- excluded: main.c, tui.c (interactive cli/render), petri_api.c (facade,
  helix ships its own driver in src/kdf.c), other connectome datasets

connectome used for key derivation: data/herm_302.csv
(302 neurons, openworm herm_full_edgelist, see engine README).

## file hashes (sha256)

762ed4e493d6ca8dae32fbe82253809c5e307095c004deab000d34adbaad694c  vendor/petri/body.c
2e946d221fa5ef1fe316111b148c48bc5c2384a4c7e6cd5be82ce6fe3802c41f  vendor/petri/connectome.c
f6da1cd09bed1151ead4b1f73e640bc6c986541081d3f6d44def35065d92f710  vendor/petri/evolution.c
8dd8a76e8a3e940156141d26e59cd6e4c18af73e34f7b99d277fcbe8fa204d3e  vendor/petri/gcamp.c
8a69d1dca5d1fdc4ebb1d44e10125b0c632f3b79d8d7ca203f7b6d5105ad383a  vendor/petri/metrics.c
88cabb66390f9d1e0118ecaa21d9d7cd35b8290d2c0945ff00585d7120291e75  vendor/petri/modulators.c
70204ae4800d8078133b36233c1a78e4fbb3510520e4ce0bb718764200aa91a4  vendor/petri/mutants.c
2aff5f49f1f4a96ac3490a018fb4f93c390d16ae3c40414d37e168e1fbdd936f  vendor/petri/neuron.c
5f63ce0cb06df27e7f4889de705b6c64f22c47595357671fb3f9691d4f9af5e0  vendor/petri/plasticity.c
d01f7b1a92ecb66a7e3334bde208374a6bfee45e91b74498c37c704bad5d47bb  vendor/petri/rng.c
b360a74400935066c60b07d8c98638bdc4b2dc8610925c183bbd3450dfe8d12c  vendor/petri/sim.c
432e0eacfb0d1e5f45c906a8c460d99cfe56ae3d79dadc7aec16f7cf0f370d9b  vendor/petri/world.c
f45f81a1630bc1c1600d59fb35b631e7961c4e9a86101128bc26e6f7ea3089ef  vendor/petri/body.h
b21fa1eb4cd71efc79810b62efb87ba5a4a4d3e24d14aed2d9c2c159dcde7509  vendor/petri/config.h
fd5577d1951f050969aaf11933d3db619a3a206933a2c45d12ddfbc069dfeab4  vendor/petri/connectome.h
ffbc6cb56a93e6f0dadf1ef33f3571133a8b208904446859c1a530d92d3618d5  vendor/petri/evolution.h
3581beb5f60a81334183e1cf7c45204da85738cee5eda8ab744e8465d71a0762  vendor/petri/gcamp.h
ed25a745406c8b943fe5d6af9818c7ab3d7371df0e85e49d1af05e15aad2f7f2  vendor/petri/metrics.h
487dc20343c1b69861b37c5ee2f7536adb133497c69721c93a08fb82e5ab3160  vendor/petri/modulators.h
e36ba1a326b68fdd848511717b82b824337a3eb60ac8efb74a4e75bf7dfe550f  vendor/petri/mutants.h
50fc1c96bc49b999a0e93006c132c9e4732179f36aadb622175615bee280702b  vendor/petri/neuron.h
965d94af4f1ae5995f68af95f723e6ab54aa3b8bb61147bb986c280d3082f85d  vendor/petri/plasticity.h
c46c6a9845ac3d2db65ce9a5312dfdf1c0cbb8e8f504dec26b8e0297ab2d7a84  vendor/petri/rng.h
5f1de5c0ad0b36d75d0e327f361b69a95bfaca12a1d7ae9d8436d44935f1bbeb  vendor/petri/sim.h
095dfb672fa6c795d3132a744817186c9e5d10e2f9a89702161037983f40dccb  vendor/petri/world.h
97909f25ee635469679633fdaab0cbaa71ab70fad87af3e4a2df584760bc8bf4  vendor/petri/data/herm_302.csv
b8fbcce33cbe055900577bb9c806c4f0f04b39f0b3dca838e0cd1e00ffd9e301  vendor/petri/LICENSE
