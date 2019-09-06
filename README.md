# kfeditor-patcher

The UE3 `mergepackages` commandlet can no longer be executed due to a bug in TripWire's code. This tool is a self-deploying bootstrapper/patcher that allows to work around this issue.

Installation Instructions
------------------------------

Save `kfeditor_patcher.exe` in your `steamapps\common\killingfloor2\Binaries\Win64` directory and execute it.

`KFEditor.exe` should get replaced by the bootstrapper and you should see two new files appear: `KFEditor_original.exe` and `KFEditor_mergepackages.exe`.

Note: the patcher has to be re-executed whenever the game gets updated.
