Run the following to start the testing dashboard:
```cmd
ctest --test-dir build -D Experimental
# Run only unit tests
ctest --test-dir build -D Experimental -L unit
# Run unit tests with only annotations
ctest --test-dir build -D Experimental -L unit -LE mem
# Run only experiments with annotations
ctest --test-dir build -D Experimental -L experiment -LE mem
# Run only experiments with only memory
ctest --test-dir build -D Experimental -L experiment -L mem
```



It will upload towards `https://my.cdash.org/overview.php?project=HaliVer`

