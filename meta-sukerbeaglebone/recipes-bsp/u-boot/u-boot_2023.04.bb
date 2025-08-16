require u-boot-common.inc
require u-boot.inc

# 로컬 Git 리포지토리 경로를 지정합니다.
# 이 경로는 meta-mylayer/recipes-bsp/u-boot 디렉토리에서 U-Boot 소스 디렉토리까지의 상대 경로여야 합니다.
# 예시: U-Boot 소스가 /home/user/workspace/u-boot 에 있는 경우,
#       S = "/home/user/workspace/u-boot"
#       git:// 경로를 사용하는 방법도 있습니다.
#       SRC_URI = "git:///home/user/workspace/u-boot"
#
# 로컬 Git 리포지토리의 절대 경로를 직접 지정하는 것이 가장 간단합니다.
# SRC_URI = "git:///path/to/your/u-boot.git"
#
# git:// 뒤에 슬래시가 3개인 것을 확인하세요. (git:///)

# Git 리포지토리의 특정 커밋이나 브랜치를 지정할 수 있습니다.
# AUTOREV를 사용하면 항상 최신 커밋을 사용합니다.
SRCREV = "${AUTOREV}"
#
# 특정 브랜치를 지정하려면 다음과 같이 합니다.
# SRCREV = "auto"
# BRANCH = "my-dev-branch"
#
# 또는 특정 커밋 해시를 지정할 수도 있습니다.
# SRCREV = "abcdef1234567890abcdef1234567890abcdef12"

# Yocto 빌드 시스템이 로컬 Git 리포지토리를 인식하도록 설정합니다.
# 이 설정은 로컬 Git 리포지토리에서 바로 빌드하기 위해 필요합니다.
# Yocto의 git.bbclass는 fetcher에서 git 리포지토리 클론 후 S(소스) 디렉토리로 이동하는 과정을 거치는데
# 이 부분을 건너뛰고 바로 소스 디렉토리에서 작업하도록 설정합니다.
#
# 이 방법은 Yocto 레시피의 `S` 변수를 오버라이드하여 로컬 소스를 직접 가리키도록 하는 방법입니다.
# 예를 들어, u-boot 소스 디렉토리가 /home/myuser/u-boot 일 경우
S = "/home/suker/myYocto/MYSRC/ti-u-boot"
B = "${WORKDIR}/build"
#
# 또는 `local.conf`에서 `BB_NO_NETWORK = "1"`와 같은 설정을 사용하고
# `git://...` 대신 `file:///...` 로컬 경로를 사용하면 네트워크 연결 없이 로컬 소스를 사용할 수 있습니다.
#
# 가장 간단한 방법은 `SRC_URI`에 로컬 Git 리포지토리의 절대 경로를 직접 지정하는 것입니다.
SRC_URI = "git:///home/suker/myYocto/MYSRC/ti-u-boot;protocol=file;branch=ti-u-boot-2023.04"

do_patch[noexec] = "1"
do_unpack[noexec] = "1"

# SRC_URI의 git:// 뒤에 슬래시가 3개인 것을 확인해야 합니다.
# git:// + /path/to/your/u-boot -> git:///path/to/your/u-boot
#
# 이 방법은 Yocto가 로컬 리포지토리를 클론하여 WORKDIR에 풀지 않고,
# 해당 경로에서 직접 소스를 가져와 작업하게 됩니다.
# 따라서 로컬에서 수정하고 커밋한 내용은 바로 빌드에 반영됩니다.

# 로컬 Git 리포지토리에서 작업할 때는 다음과 같은 변수들을 설정하는 것이 좋습니다.
# 빌드 시 로컬에서 변경된 내용이 패치로 생성되는 것을 방지합니다.
PV = "2023.04.LOCAL"
SRC_URI_OVERRIDES_PV = "1"
#LOCAL_SRC_DIR = ${B} # 로컬 소스 디렉토리에서 바로 빌드하도록 B(빌드 디렉토리)를 S(소스 디렉토리)로 설정합니다.

DEPENDS += "bc-native dtc-native python3-setuptools-native"
