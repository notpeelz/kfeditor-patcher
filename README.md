# kfeditor-patcher

The UE3 `mergepackages` commandlet can no longer be executed due to a bug in TripWire's code. This patch allows to work around this issue.


Installation Instructions
------------------------------

Save `kfeditor_patcher.exe` to your `steamapps\common\killingfloor2\Binaries\Win64` directory and execute it.

Note: the patcher has to be re-executed whenever the game gets updated.


Usage
-------

`kfeditor_mergepackages.exe make SourceUPK.upk DestinationUPK.upk`

The resulting UPK will appear in `steamapps\common\killingfloor2\Binaries\Win64`.

Notes:
- You have to launch it using the `make` commandlet, not `mergepackages`. This is not a typo.
- You might have to move your assets to `steamapps\common\killingfloor2\KFGame\BrewedPC` or the commandlet might not correctly resolve package references.
