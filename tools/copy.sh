files=('yolov8n_pnnx.py.ncnn.bin' 'yolov8n_pnnx.py.ncnn.param')

for file in "${files[@]}"; do
    echo "copy $file"
    cp models/$file assets/
done
