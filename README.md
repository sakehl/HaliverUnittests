# Run tests
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


# Experiments
## Run experiments
Use 
```
python3 experiments/run_experiments.py
```
## View experiments
Use 
```
python3 -m http.server
```
To start a http server to view the results at
```
experiments/results/exp-2025-03-18.xml
```
## Make table
```
python3 experiments/xml_to_latex.py
```


# TODO:
* reduction_bug.cpp
* reduction_rfactor.cpp