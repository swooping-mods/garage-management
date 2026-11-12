# Garage Management: behavior and implementation

Describes the retained payload **0.1.0**. The behavior prose describes the retained payload; current candidate source and metadata are identified below. Gameplay qualification is a separate requirement.


<!-- source-only-sync -->
## Current source candidate

Source version **0.1.0** is saved from workshop revision `ca32bbf79ce0840d5166a232bdb3a9fdcd761254`. The retained installable payload remains **0.1.0** with its original bytes. This source update does not rebuild or requalify that payload. The current source and metadata define the candidate; gameplay, rendered behavior and co-op remain unqualified for rebuilt bytes.

The authored behavior and lifecycle descriptions below are retained from the published payload. They are not new qualification evidence for the source candidate. The integration table below follows the current candidate metadata.
<!-- /source-only-sync -->

## Behavior

Extends native Fleet/Upgrades summaries using exact owned storage identities rather than model-first lookups. It retains up to 128 local owned records. Alt+PageUp/PageDown selects a fleet record. Alt+1…7, repeated within five seconds, confirms native Speed/Hopper/Turbo/Cruise/Suction/Unload/Lights upgrades using current prices, levels and expected-level checks. Native fleet clicks and store/recall/install buttons remain game-owned.

## State and lifecycle

Selection roots and confirmation timers are temporary. Purchases/upgrades use native owned-record storage and normal saves, including multiple same-model vehicles. Host-only actions revalidate owner and instance; focus, typing and menu context gate shortcuts. World changes release the fleet snapshot and pending confirmation.

## Integration and configuration

No additional feature-mod runtime dependency is declared. Included shared headers are source reuse, not extra DLL dependencies.

The [module metadata](../source/mods/garage-management/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

## Implementation sources

- [mod.c](../source/mods/garage-management/mod.c)
- [base.h](../source/mods/garage-management/base.h)

## Verification boundary

No separate vehicle storage format or bulk-upgrade transaction. Native UI layout, exact-instance upgrades, economy replay and co-op need qualification.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
