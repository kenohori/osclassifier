rm ./osmm_*.gpkg
ogr2ogr -where "citygmlclass = 'Building'" -f GPKG osmm_building.gpkg osmm.gpkg
ogr2ogr -where "citygmlclass = 'PlantCover'" -f GPKG osmm_plantcover.gpkg osmm.gpkg
ogr2ogr -where "citygmlclass = 'Railway'" -f GPKG osmm_railway.gpkg osmm.gpkg
ogr2ogr -where "citygmlclass = 'Road'" -f GPKG osmm_road.gpkg osmm.gpkg
ogr2ogr -where "citygmlclass = 'WaterBody'" -f GPKG osmm_waterbody.gpkg osmm.gpkg
ogr2ogr -where "citygmlclass = 'LandUse'" -f GPKG osmm_landuse.gpkg osmm.gpkg
ogr2ogr -where "citygmlclass = 'Bridge'" -f GPKG osmm_bridge.gpkg osmm.gpkg