# 设置 rustup 国内源
export RUSTUP_DIST_SERVER="https://mirrors.ustc.edu.cn/rust-static"
export RUSTUP_UPDATE_ROOT="https://mirrors.ustc.edu.cn/rust-static/rustup"

export RUSTUP_HOME=$HOME/.rustup
export CARGO_HOME=$HOME/.cargo
export PATH=$PATH:$CARGO_HOME/bin

if [[ -f $CARGO_HOME/env ]]; then
    source $CARGO_HOME/env
    exit 0
fi

detect_osname() {
    local os_name="unknown"

    # 检查环境变量 OS 或 OSTYPE
    if [ -n "$OS" ]; then
        os_name="$OS"
    elif [ -n "$OSTYPE" ]; then
        os_name="$OSTYPE"
    fi

    # 判断操作系统类型
    if [ "$os_name" = "Windows_NT" ]; then
        echo "Windows"
    elif [[ "$os_name" == *"linux"* ]]; then
        echo "Linux"
    elif [[ "$os_name" == *"darwin"* ]] || [ "$os_name" = "Darwin" ]; then
        echo "macOS"
    else
        # 回退到 uname 命令
        if command -v uname >/dev/null 2>&1; then
            uname_out=$(uname -s 2>/dev/null)
            if [ "$uname_out" = "Darwin" ]; then
                echo "macOS"
            elif [ "$uname_out" = "Linux" ]; then
                echo "Linux"
            else
                echo "$uname_out"
            fi
        else
            echo "unknown"
        fi
    fi
}
# 1. 卸载 brew 安装的 rust
os=$(detect_osname)

if [[ $os == "macOS" ]]; then
    brew uninstall rust
elif [[ $os == "Linux" ]]; then
    sudo apt remove rust
else
    choco uninstall rust
fi

# 2. 安装 rustup（如果还没装）
if command -v rustup >/dev/null 2>&1; then
    echo "rustup is installed at: $(which rustup)"
else
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
fi

# 3. 激活环境
if [[ ! -d $HOME/.cargo ]]; then
    mkdir -p $HOME/.cargo
fi

# 4. 设置默认工具链
rustup default stable

# 5. 验证
if [[ ! -f "$HOME/.cargo/env" ]]; then
    rustup-init
fi
rustc --version
cargo --version

rustup target add aarch64-linux-android

if [[ -f "$HOME/.cargo/env" ]]; then
    source "$HOME/.cargo/env"
fi
