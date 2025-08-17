# Structure
This file describes file structure that is present in this repo. Please note that the repo uses git submodules!

/yap
|- modules
|  |- module1 
|  |- module2 
|  |- module3 
|- src (This contains the source code for the project)
|- lib (Contains libraries, will be filled during installation) 
|- bin (Place where all final binaries land)
|- README.md (A quick insight into the project)
|- structure.md (This file)
|- makefile (Used to build the project)

## Modules
Yapl is supposed to be very modular. You can freely exchange front and back end as well as add extra capabilites to the 'yap' command.

By default, yap will use yap-ts as the front end and yap-c as the backend.

