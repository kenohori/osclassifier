# osclassifier

Simple C++ script to reclassify Ordnance Survey [MasterMap](https://www.ordnancesurvey.co.uk/business-government/products/mastermap-topography) topographic data to the classes in the [CityJSON](https://www.cityjson.org) 3D city modelling standard.

In order to use this, you need C++17 or higher and the [GDAL](https://gdal.org) library. A CMake file is provided.

Just edit lines 9 and 10 to specify your input and output files. The appropriate CityJSON classes will be added as an additional attribute `cityjsonclass`.
