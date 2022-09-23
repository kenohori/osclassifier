#include <iostream>
#include <fstream>
#include <set>

#include <ogrsf_frmts.h>

int main(int argc, const char * argv[]) {
  
  const char *inputFile = "/Users/ken/Downloads/vector_data/os_mastermap_topo_exeter_small_aoi.gpkg";
  const char *outputFile = "/Users/ken/Downloads/3dfier_os/osmm.gpkg";

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
    if (strcmp(inputLayer->GetName(), "osmm_topo_topographicarea") != 0) {
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
    } OGRFieldDefn citygmlClassField("citygmlclass", OFTString);
    citygmlClassField.SetWidth(25);
    if (outputLayer->CreateField(&citygmlClassField) != OGRERR_NONE) {
      std::cout << "Error: Could not create field citygmlclass." << std::endl;
      return 1;
    }
    
    OGRFeature *inputFeature;
    std::set<std::string> unclassifiedDescriptiveGroups, unclassifiedDescriptiveTerms;
    unsigned long long unclassifiedFeatures = 0;
    while ((inputFeature = inputLayer->GetNextFeature()) != NULL) {
      if (!inputFeature->GetGeometryRef()) continue;
      if (inputFeature->GetGeometryRef()->getGeometryType() == wkbPolygon ||
          inputFeature->GetGeometryRef()->getGeometryType() == wkbPolygon25D) {
        OGRFeature *outputFeature = OGRFeature::CreateFeature(outputLayer->GetLayerDefn());
        for (int currentField = 0; currentField < inputFeature->GetFieldCount(); currentField++) {
          outputFeature->SetField(currentField, inputFeature->GetRawFieldRef(currentField));
        }
        
        int descriptiveGroupField = inputFeature->GetFieldIndex("descriptivegroup");
        int descriptiveTermField = inputFeature->GetFieldIndex("descriptiveterm");
        int makeField = inputFeature->GetFieldIndex("make");
        
        std::string descriptiveGroup = inputFeature->GetFieldAsString(descriptiveGroupField);
        std::string descriptiveTerm = inputFeature->GetFieldAsString(descriptiveTermField);
        std::string make = inputFeature->GetFieldAsString(makeField);
        
        if (descriptiveGroup == "{Building}") outputFeature->SetField("citygmlclass", "Building");
        else if (descriptiveGroup == "{Structure}" && make == "Manmade") outputFeature->SetField("citygmlclass", "Building");
        else if (descriptiveTerm == "{Weir}") outputFeature->SetField("citygmlclass", "Building");

        else if (descriptiveGroup == "{Rail}") outputFeature->SetField("citygmlclass", "Railway");

        else if (descriptiveGroup == "{\"Road Or Track\"}") outputFeature->SetField("citygmlclass", "Road");
        else if (descriptiveGroup == "{Path}") outputFeature->SetField("citygmlclass", "Road"); // for pedestrians
        else if (descriptiveGroup == "{Roadside}" && make == "Manmade") outputFeature->SetField("citygmlclass", "Road"); // pavement
        else if (descriptiveGroup == "{Roadside}" && make == "Unknown") outputFeature->SetField("citygmlclass", "Road"); // pedestrian islands
        else if (descriptiveGroup == "{Roadside,Structure}") outputFeature->SetField("citygmlclass", "Road"); // protection for pedestrian crossings

        else if (descriptiveGroup == "{\"Inland Water\"}") outputFeature->SetField("citygmlclass", "WaterBody");

        else if (descriptiveGroup == "{\"Natural Environment\"}") outputFeature->SetField("citygmlclass", "PlantCover");
        else if (descriptiveGroup == "{\"Natural Environment\",Rail}") outputFeature->SetField("citygmlclass", "PlantCover");
        else if (descriptiveGroup == "{\"Natural Environment\",Roadside}") outputFeature->SetField("citygmlclass", "PlantCover");
        else if (make == "Natural") outputFeature->SetField("citygmlclass", "PlantCover");
        else if (descriptiveGroup == "{Landform}") outputFeature->SetField("citygmlclass", "PlantCover");
        else if (descriptiveGroup == "{Landform,Rail}") outputFeature->SetField("citygmlclass", "PlantCover");
        else if (descriptiveGroup == "{\"Historical Interest\",Rail}") outputFeature->SetField("citygmlclass", "PlantCover");

        else if (descriptiveTerm == "{Bridge}") outputFeature->SetField("citygmlclass", "Bridge");
        else if (descriptiveTerm == "{Footbridge}") outputFeature->SetField("citygmlclass", "Bridge");
//        else if (descriptiveGroup == "{\"Road Or Track\",Structure}") outputFeature->SetField("citygmlclass", "Bridge");
//        else if (descriptiveGroup == "{Roadside,Structure}") outputFeature->SetField("citygmlclass", "Bridge");
//        else if (descriptiveGroup == "{Path,Structure}") outputFeature->SetField("citygmlclass", "Bridge");
//        else if (descriptiveGroup == "{Rail,Structure}") outputFeature->SetField("citygmlclass", "Bridge");
//        else if (descriptiveGroup == "{\"General Surface\",Structure}") outputFeature->SetField("citygmlclass", "Bridge");

        else if (descriptiveTerm == "{\"Multi Surface\"}") outputFeature->SetField("citygmlclass", "LandUse");
        else if (descriptiveTerm == "{Slope}") outputFeature->SetField("citygmlclass", "LandUse");
        else if (descriptiveGroup == "{\"General Surface\"}" && make == "Manmade") outputFeature->SetField("citygmlclass", "LandUse");
//        else if (descriptiveGroup == "{\"General Surface\"}" && make == "Multiple") outputFeature->SetField("citygmlclass", "LandUse");
//        else if (descriptiveGroup == "{Roadside}" && make == "Unknown") outputFeature->SetField("citygmlclass", "LandUse");
        else if (descriptiveGroup == "{Unclassified}") outputFeature->SetField("citygmlclass", "LandUse");
        
        else {
          unclassifiedDescriptiveGroups.insert(inputFeature->GetFieldAsString(descriptiveGroupField));
          unclassifiedDescriptiveTerms.insert(inputFeature->GetFieldAsString(descriptiveTermField));
          ++unclassifiedFeatures;
          outputFeature->SetField("citygmlclass", "");
        }
        
        outputFeature->SetGeometry(inputFeature->GetGeometryRef());
        if (outputLayer->CreateFeature(outputFeature) != OGRERR_NONE) {
          std::cerr << "Error: Could not create feature." << std::endl;
          return 1;
        } OGRFeature::DestroyFeature(outputFeature);
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
