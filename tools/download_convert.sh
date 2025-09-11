
function install_requirements() {
    if [ -d "venv" ]; then
        echo "evnv exists"
    else
        python3 -m venv venv
    fi

    source venv/bin/activate
    pip install -r tools/requirements.txt
}


function download_models() {
    if [[ ! -d models ]]; then
        mkdir models
    fi

    cd models
    if [[ ! -f yolov8n.pt ]]; then
        echo "### download models"
        yolo export model=yolov8n.pt format=torchscript
    fi
    cd -
}

function convert_models() {
    echo "### convert models"
    cd models
    if [ ! -f yolov8n.pnnx.param ]; then
        pnnx yolov8n.torchscript
    fi
    cd -

}

function rewrite_script() {
    cd models
    text_from=( 
        "v_165 = v_142.view(1, 144, 6400)", \
        "v_166 = v_153.view(1, 144, 1600)", \
        "v_167 = v_164.view(1, 144, 400)", \
        "v_168 = torch.cat((v_165, v_166, v_167), dim=2)", \
        "v_169, v_170 = torch.split(tensor=v_168, dim=1, split_size_or_sections=(64,80))" \
        )

    text_to=( \
        "v_165 = v_142.view(1, 144, -1).transpose(1, 2)", \
        "v_166 = v_153.view(1, 144, -1).transpose(1, 2)", \
        "v_167 = v_164.view(1, 144, -1).transpose(1, 2)", \
        "v_168 = torch.cat((v_165, v_166, v_167), dim=1)", \
        "return v_168" \
        )

    echo "### rewrite yolov8n_pnnx.py"
    for i in "${!text_from[@]}"; do
        echo "replacing ${text_from[i]} with ${text_to[i]}"
        local from_text="${text_from[i]}"
        local to_text="${text_to[i]}"

        # Escape special characters in sed
        local escaped_from=$(echo "$from_text" | sed 's/[/&/\]/\\&/g')
        local escaped_to=$(echo "$to_text" | sed 's/[/&/\]/\\&/g')

        # Use @ as delimiter to avoid conflicts
        sed -i "s@${escaped_from}@${escaped_to}@g" yolov8n_pnnx.py
    done
    sed -i "s@v_165 = v_142.view(1, 144, 6400)@v_165 = v_142.view(1, 144, -1).transpose(1, 2)@g" yolov8n_pnnx.py
    sed -i "s@v_166 = v_153.view(1, 144, 1600)@v_166 = v_153.view(1, 144, -1).transpose(1, 2)@g" yolov8n_pnnx.py
    sed -i "s@v_167 = v_164.view(1, 144, 400)@v_167 = v_164.view(1, 144, -1).transpose(1, 2)@g" yolov8n_pnnx.py
    sed -i "s@v_168 = torch.cat((v_165, v_166, v_167), dim=2)@v_168 = torch.cat((v_165, v_166, v_167), dim=1)@g" yolov8n_pnnx.py


    cd -
}

function re_export() {
    echo "### re_export yolov8n_pnnx.py.pt"
    cd models
    python3 -c 'import yolov8n_pnnx; yolov8n_pnnx.export_torchscript()'
    cd -
}

function re_convert() {
    echo "### re_convert yolov8n_pnnx.py.pt"
    cd models
    pnnx yolov8n_pnnx.py.pt inputshape=[1,3,640,640] inputshape2=[1,3,320,320]
    cd -
}

install_requirements
download_models
convert_models
rewrite_script
re_export
re_convert



