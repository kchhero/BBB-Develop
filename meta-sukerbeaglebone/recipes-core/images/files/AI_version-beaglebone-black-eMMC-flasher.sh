#!/bin/bash
#
# BeagleBone Black eMMC 플래싱 스크립트 (Yocto)
# SD카드로 부팅 후 실행하면 eMMC 전체를 안전하게 덮어씁니다.
# 사용 전 반드시 /dev/mmcblkX 경로를 확인하세요.
#

set -e

SRC_DEV="/dev/mmcblk0"   # SD 카드 (부팅 디바이스)
DST_DEV="/dev/mmcblk1"   # eMMC
BOOT_PART="${SRC_DEV}p1"
ROOT_PART="${SRC_DEV}p2"

echo "========================================"
echo " BeagleBone Black eMMC Flasher (Yocto)"
echo "========================================"
echo "SD 카드:   $SRC_DEV"
echo "eMMC:      $DST_DEV"
echo "----------------------------------------"
echo "!!! 경고: eMMC(${DST_DEV}) 내용이 모두 삭제됩니다 !!!"
echo "5초 안에 Ctrl+C 로 취소 가능"
sleep 5

# 1. eMMC 초기화
echo "[1/5] eMMC 초기화..."
blkdiscard ${DST_DEV} || true

# 2. 파티션 생성 (부트, 루트)
echo "[2/5] eMMC 파티션 생성..."
parted --script ${DST_DEV} mklabel msdos
parted --script ${DST_DEV} mkpart primary fat16 1MiB 129MiB
parted --script ${DST_DEV} set 1 boot on
parted --script ${DST_DEV} mkpart primary ext4 129MiB 100%

# 3. 파일시스템 생성
echo "[3/5] 파일시스템 생성..."
mkfs.vfat -F 32 -n BOOT ${DST_DEV}p1
mkfs.ext4 -F -L rootfs ${DST_DEV}p2

# 4. 데이터 복사
echo "[4/5] 데이터 복사..."
mkdir -p /mnt/src_boot /mnt/src_root /mnt/dst_boot /mnt/dst_root

mount ${BOOT_PART} /mnt/src_boot
mount ${ROOT_PART} /mnt/src_root
mount ${DST_DEV}p1 /mnt/dst_boot
mount ${DST_DEV}p2 /mnt/dst_root

echo "부트 파티션 복사..."
rsync -aAX --delete /mnt/src_boot/ /mnt/dst_boot/

echo "루트 파티션 복사..."
rsync -aAX --delete /mnt/src_root/ /mnt/dst_root/

sync

# 5. 마무리
echo "[5/5] 마운트 해제..."
umount /mnt/src_boot /mnt/src_root /mnt/dst_boot /mnt/dst_root

echo "========================================"
echo " eMMC 플래싱 완료!"
echo " 전원을 끄고 SD카드를 제거한 뒤 eMMC로 부팅하세요."
echo "========================================"