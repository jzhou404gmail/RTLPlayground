#!/bin/bash
set -u
DIR=/vol1/Public
CONF=/etc/samba/smb.custom.conf
MARK="# --- DSH-PUBLIC-SHARE ---"

id -u | grep -qx 0 || { echo "ERROR: 需要 root"; exit 1; }

echo "===== 1) 创建目录 ====="
if [ ! -d "$DIR" ]; then
    mkdir -p "$DIR" || { echo "ERROR: mkdir 失败"; exit 1; }
    echo "CREATED: $DIR"
else
    echo "EXISTS : $DIR"
fi
# 匿名访问由 guest 账号(nobody:65534)进行，必须让它可读写
chown nobody:nogroup "$DIR" || { echo "ERROR: chown 失败"; exit 1; }
chmod 0777 "$DIR"
ls -ld "$DIR"

echo
echo "===== 2) 添加共享定义 ====="
if grep -qF "$MARK" "$CONF"; then
    echo "SKIP: 定义已存在"
else
    BAK="$CONF.bak-dsh-public-$(date +%Y%m%d-%H%M%S)"
    cp -a "$CONF" "$BAK" || { echo "ERROR: 备份失败"; exit 1; }
    echo "BACKUP -> $BAK"
    cat >> "$CONF" <<INNER

$MARK
[Public]
	path = $DIR
	browseable = yes
	available = yes
	guest ok = yes
	read only = no
	create mask = 0666
	directory mask = 0777
INNER
    echo "APPENDED: [Public]"
fi

echo
echo "===== 3) $CONF 内容 ====="
cat "$CONF"

echo
echo "===== 4) testparm 校验 ====="
testparm -s /etc/samba/smb.conf > /tmp/dsh_tp.out 2> /tmp/dsh_tp.err
echo "rc=$?"
echo "--- 解析出的 [Public] 段 ---"
sed -n '/^\[Public\]/,/^$/p' /tmp/dsh_tp.out
echo "--- stderr ---"
cat /tmp/dsh_tp.err
