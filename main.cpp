#include <iostream>
#include <fstream>
#include <set>

#include <ogrsf_frmts.h>

int main(int argc, const char * argv[]) {
  
  const char *inputFile = "os_mastermap_topo_exeter_small_aoi.gpkg";
  const char *outputFile = "osmm.gpkg";

  // Prepare input file
  GDALAllRegister();
  GDALDataset *inputDataset = (GDALDataset*) GDALOpenEx(inputFile, GDAL_OF_READONLY, NULL, NULL, NULL);
  if (inputDataset == NULL) {
    std::cerr << "Error: Could not open input file." << std::endl;
    return 1;
  }
  
  // Prepare output file
  std::filesystem::path extension = std::filesystem::path(outputFile).extension();
  const char *driverName;
  if (extension.compare(".csv") == 0) driverName = "CSV";
  else if (extension.compare(".dxf") == 0) driverName = "DXF";
  else if (extension.compare(".gdb") == 0) driverName = "FileGDB";
  else if (extension.compare(".json") == 0) driverName = "GeoJSON";
  else if (extension.compare(".geojson") == 0) driverName = "GeoJSON";
  else if (extension.compare(".gml") == 0) driverName = "GML";
  else if (extension.compare(".gpkg") == 0) driverName = "GPKG";
  else if (extension.compare(".kml") == 0) driverName = "KML";
  else if (extension.compare(".shp") == 0) driverName = "ESRI Shapefile";
  GDALDriver *driver = GetGDALDriverManager()->GetDriverByName(driverName);
  if (driver == NULL) {
    std::cerr << "Error: " << driverName << " driver not found." << std::endl;
    return 1;
  } GDALDataset *outputDataset = (GDALDataset*) GDALOpenEx(outputFile, GDAL_OF_READONLY, NULL, NULL, NULL);
  if (outputDataset != NULL) {
    std::cout << "Overwriting file..." << std::endl;
    if (driver->Delete(outputFile)!= CE_None) {
      std::cerr << "Error: Couldn't erase existing file to create output file" << std::endl;
      return 1;
    } GDALClose(outputDataset);
  }
  
  std::cout << "Writing output file... " << std::endl;
  outputDataset = driver->Create(outputFile,0,0,0,GDT_Unknown,NULL);
  if (outputDataset == NULL) {
    std::cout << "Error: Could not create output file." << std::endl;
    return 1;
  }
  
  std::cout << "Input file info:" << std::endl;
  std::cout << "\tPath: " << inputFile << std::endl;
  std::cout << "\tType: " << inputDataset->GetDriverName() << std::endl;
  std::cout << "\tLayers: " << inputDataset->GetLayerCount() << std::endl;
  
  for (auto &&inputLayer: inputDataset->GetLayers()) {
    std::cout << "Reading layer " << inputLayer->GetName() << " (" << inputLayer->GetFeatureCount(true) << " features)..." << std::endl;
    if (strcmp(inputLayer->GetName(), "osmm_topo_topographicarea") != 0 &&
        strcmp(inputLayer->GetName(), "output") != 0) {
      std::cout << "\t->skipped" << std::endl;
      continue;
    }
    
    inputLayer->ResetReading();
    OGRSpatialReference* spatialReference = NULL;
    if (inputLayer->GetSpatialRef() != NULL) {
      spatialReference = inputLayer->GetSpatialRef()->Clone();
    }
    
    OGRLayer *outputLayer = outputDataset->CreateLayer(inputLayer->GetName(), spatialReference, wkbPolygon, NULL);
    if (outputLayer == NULL) {
      std::cerr << "Error: Could not create output layer." << std::endl;
      return 1;
    }
    
    OGRFeatureDefn *layerDefinition = inputLayer->GetLayerDefn();
    for (int currentField = 0; currentField < layerDefinition->GetFieldCount(); currentField++) {
      if (outputLayer->CreateField(layerDefinition->GetFieldDefn(currentField)) != OGRERR_NONE) {
        std::cerr << "Error: Could not create field " << layerDefinition->GetFieldDefn(currentField)->GetNameRef() << "." << std::endl;
        return 1;
      }
    } OGRFieldDefn cityjsonClassField("cityjsonclass", OFTString);
    cityjsonClassField.SetWidth(25);
    if (outputLayer->CreateField(&cityjsonClassField) != OGRERR_NONE) {
      std::cout << "Error: Could not create field cityjsonclass." << std::endl;
      return 1;
    }
    
    OGRFeature *inputFeature;
    std::set<std::string> unclassifiedDescriptiveGroups, unclassifiedDescriptiveTerms;
    unsigned long long unclassifiedFeatures = 0;
    while ((inputFeature = inputLayer->GetNextFeature()) != NULL) {
      if (!inputFeature->GetGeometryRef()) {
        std::cout << "Skipped feature with no geometry." << std::endl;
        continue;
      }
      if (inputFeature->GetGeometryRef()->getGeometryType() == wkbPolygon ||
          inputFeature->GetGeometryRef()->getGeometryType() == wkbMultiPolygon) {
        
        int descriptiveGroupField = inputFeature->GetFieldIndex("descriptivegroup");
        int descriptiveTermField = inputFeature->GetFieldIndex("descriptiveterm");
        int makeField = inputFeature->GetFieldIndex("make");
        
        std::string descriptiveGroup = inputFeature->GetFieldAsString(descriptiveGroupField);
        std::string descriptiveTerm = inputFeature->GetFieldAsString(descriptiveTermField);
        std::string make = inputFeature->GetFieldAsString(makeField);
        std::string cityjsonClass;
        
        // Straightforward mappings
        if (descriptiveGroup == "{Building}") cityjsonClass = "Building";
        else if (descriptiveGroup == "{Building,Structure}") cityjsonClass = "Building";
        else if (descriptiveGroup == "{Building,Rail}") cityjsonClass = "Building";
        else if (descriptiveGroup == "{Building,\"Road Or Track\"}") cityjsonClass = "Building";
        else if (descriptiveTerm == "{Weir}") cityjsonClass = "Building";
        else if (descriptiveTerm == "{Foreshore,Weir}") cityjsonClass = "Building";
        else if (descriptiveTerm == "{Cross}") cityjsonClass = "Building";
        else if (descriptiveGroup == "{Glasshouse}") cityjsonClass = "Building";

        else if (descriptiveGroup == "{Rail}") cityjsonClass = "Railway";
        else if (descriptiveTerm == "{\"Level Crossing\"}") cityjsonClass = "Railway";

        else if (descriptiveGroup == "{\"Road Or Track\"}") cityjsonClass = "Road";
        else if (descriptiveGroup == "{\"General Surface\",\"Road Or Track\"}") cityjsonClass = "Road";
        else if (descriptiveGroup == "{\"Road Or Track\",\"General Feature\"}") cityjsonClass = "Road";
        else if (descriptiveGroup == "{Path}") cityjsonClass = "Road"; // for pedestrians
        else if (descriptiveTerm == "{Track}") cityjsonClass = "Road";
        else if (descriptiveTerm == "{Foreshore,Step}") cityjsonClass = "Road";

        else if (descriptiveGroup == "{\"Inland Water\"}") cityjsonClass = "WaterBody";
        else if (descriptiveGroup == "{\"Inland Water\",Structure}") cityjsonClass = "WaterBody";
        else if (descriptiveGroup == "{\"Tidal Water\"}") cityjsonClass = "WaterBody";
        else if (descriptiveGroup == "{\"Natural Environment\",\"Tidal Water\"}") cityjsonClass = "WaterBody";
        else if (descriptiveGroup == "{\"Inland Water\",\"Natural Environment\"}") cityjsonClass = "WaterBody";
        else if (descriptiveGroup == "{\"Inland Water\",\"Natural Environment\"}") cityjsonClass = "WaterBody";
        else if (descriptiveGroup == "{\"General Surface\",\"Inland Water\"}") cityjsonClass = "WaterBody";
        else if (descriptiveGroup == "{Structure,\"Inland Water\"}") cityjsonClass = "WaterBody";
        else if (descriptiveGroup == "{\"Inland Water\",\"Road Or Track\"}") cityjsonClass = "WaterBody";

        else if (descriptiveGroup == "{\"Natural Environment\"}") cityjsonClass = "PlantCover";
        else if (descriptiveGroup == "{\"Natural Environment\",Rail}") cityjsonClass = "PlantCover";
        else if (descriptiveGroup == "{\"Natural Environment\",Roadside}") cityjsonClass = "PlantCover";
        else if (descriptiveGroup == "{Landform}") cityjsonClass = "PlantCover";
        else if (descriptiveGroup == "{Landform,Rail}") cityjsonClass = "PlantCover";
        else if (descriptiveGroup == "{\"Historical Interest\",Rail}") cityjsonClass = "PlantCover";

        else if (descriptiveTerm == "{Bridge}") cityjsonClass = "Bridge";
        else if (descriptiveTerm == "{Footbridge}") cityjsonClass = "Bridge";
        else if (descriptiveTerm == "{Footbridge,Step}") cityjsonClass = "Bridge";
        else if (descriptiveTerm == "{\"Rail Signal Gantry\"}") cityjsonClass = "Bridge";
        else if (descriptiveTerm == "{Step}") cityjsonClass = "Bridge";
        
        else if (descriptiveTerm == "{Slipway}") cityjsonClass = "LandUse";
        else if (descriptiveTerm == "{Foreshore,Slipway}") cityjsonClass = "LandUse";
        else if (descriptiveTerm == "{Foreshore}") cityjsonClass = "LandUse";
        else if (descriptiveTerm == "{Foreshore,Sand}") cityjsonClass = "LandUse";
        else if (descriptiveGroup == "{\"General Surface\",\"Tidal Water\"}") cityjsonClass = "LandUse";
        else if (descriptiveGroup == "{Landform,\"Road Or Track\"}") cityjsonClass = "LandUse";
        else if (descriptiveGroup == "{Structure,\"Tidal Water\"}") cityjsonClass = "LandUse";
        
        // Catch other cases
        else if (descriptiveGroup == "{Structure}" && make == "Manmade") cityjsonClass = "Building";
        else if (descriptiveGroup == "{\"General Surface\",Structure}") cityjsonClass = "Building";
        
        else if (descriptiveGroup == "{Roadside}" && make == "Manmade") cityjsonClass = "Road"; // pavement
        else if (descriptiveGroup == "{Path,Roadside}") cityjsonClass = "Road"; // for pedestrians
        else if (descriptiveGroup == "{Roadside}" && make == "Unknown") cityjsonClass = "Road"; // pedestrian islands
        else if (descriptiveGroup == "{Roadside,Structure}") cityjsonClass = "Road"; // protection for pedestrian crossings
        else if (descriptiveGroup == "{\"Road Or Track\",Structure}") cityjsonClass = "Road";
        else if (descriptiveGroup == "{Structure,Path}") cityjsonClass = "Road";
        
        else if (descriptiveGroup == "{\"General Surface\"}" && make == "Natural") cityjsonClass = "PlantCover";
        else if (descriptiveGroup == "{Roadside}" && make == "Natural") cityjsonClass = "PlantCover";
        
        else if (descriptiveGroup == "{Path,Structure}") cityjsonClass = "Bridge";
        else if (descriptiveGroup == "{Rail,Structure}") cityjsonClass = "Bridge";
        else if (descriptiveGroup == "{\"Natural Environment\",Structure}") cityjsonClass = "Bridge";
        else if (descriptiveGroup == "{\"Natural Environment\",Rail,Structure}") cityjsonClass = "Bridge";
        
        else if (descriptiveTerm == "{\"Multi Surface\"}") cityjsonClass = "LandUse";
        else if (descriptiveTerm == "{Slope}") cityjsonClass = "LandUse";
        else if (descriptiveGroup == "{\"General Surface\"}" && make == "Manmade") cityjsonClass = "LandUse";
        else if (descriptiveGroup == "{Unclassified}") cityjsonClass = "LandUse";
        
        // Doesn't fit yet
        else {
          unclassifiedDescriptiveGroups.insert(inputFeature->GetFieldAsString(descriptiveGroupField));
          unclassifiedDescriptiveTerms.insert(inputFeature->GetFieldAsString(descriptiveTermField));
          ++unclassifiedFeatures;
          cityjsonClass = "";
        }
        
        if (inputFeature->GetGeometryRef()->getGeometryType() == wkbPolygon) {
          OGRFeature *outputFeature = OGRFeature::CreateFeature(outputLayer->GetLayerDefn());
          for (int currentField = 0; currentField < inputFeature->GetFieldCount(); currentField++) {
            outputFeature->SetField(currentField, inputFeature->GetRawFieldRef(currentField));
          } outputFeature->SetField("cityjsonclass", cityjsonClass.c_str());
          outputFeature->SetGeometry(inputFeature->GetGeometryRef());
          if (outputLayer->CreateFeature(outputFeature) != OGRERR_NONE) {
            std::cerr << "Error: Could not create feature." << std::endl;
            return 1;
          } OGRFeature::DestroyFeature(outputFeature);
        } else if (inputFeature->GetGeometryRef()->getGeometryType() == wkbMultiPolygon) {
          OGRMultiPolygon *inputGeometry = inputFeature->GetGeometryRef()->toMultiPolygon();
          for (int polygon = 0; polygon < inputGeometry->getNumGeometries(); ++polygon) {
            OGRFeature *outputFeature = OGRFeature::CreateFeature(outputLayer->GetLayerDefn());
            for (int currentField = 0; currentField < inputFeature->GetFieldCount(); currentField++) {
              outputFeature->SetField(currentField, inputFeature->GetRawFieldRef(currentField));
            } outputFeature->SetField("cityjsonclass", cityjsonClass.c_str());
            outputFeature->SetGeometry(inputGeometry->getGeometryRef(polygon));
            if (outputLayer->CreateFeature(outputFeature) != OGRERR_NONE) {
              std::cerr << "Error: Could not create feature." << std::endl;
              return 1;
            } OGRFeature::DestroyFeature(outputFeature);
          }
        }
        
        
      }
      
      else {
        std::cout << "Skipped feature with unknown geometry type." << std::endl;
      }
    }
    
    std::cout << unclassifiedFeatures << " unclassified features from groups:" << std::endl;
    for (auto const &descriptiveGroup: unclassifiedDescriptiveGroups) std::cout << "\t" << descriptiveGroup << std::endl;
    std::cout << "and terms:" << std::endl;
    for (auto const &descriptiveTerm: unclassifiedDescriptiveTerms) std::cout << "\t" << descriptiveTerm << std::endl;
  }
  
  GDALClose(inputDataset);
  GDALClose(outputDataset);
  
  return 0;
}
