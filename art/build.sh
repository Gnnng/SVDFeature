#!/bin/bash
build="C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe"

"$build" ./svd_feature_infer/svd_feature_infer/svd_feature_infer.vcxproj /property:Configuration=Release
# "$build" ./svd_feature_infer/svd_feature_infer.sln 

mv ./svd_feature_infer/svd_feature_infer/Release/svd_feature_infer.exe ../

