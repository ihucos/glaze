#!/bin/bash
set -eu
glaze_binary="$1"; shift
infile="$1"; shift
outfile="$1"; shift
name="$1"; shift
tmp=$(mktemp -d)
mkdir -pm 755 "$tmp/root/opt/$name/bin" "$tmp/root/opt/$name/rootfs/dev"
cp "$RUNNER_BINARY" "$tmp/root/opt/$name/glaze"
tar -xpf  "$infile" -C "$tmp/root/opt/$name/rootfs" --exclude ./dev --exclude /dev
rm     "$tmp/root/opt/$name/rootfs/etc/resolv.conf" 2> /dev/null || true
touch  "$tmp/root/opt/$name/rootfs/etc/resolv.conf"
chmod 755 "$tmp/root/opt/$name/rootfs"
cd "$tmp/root/opt/$name/rootfs"
find ./usr/local/bin ./usr/bin ./bin ./usr/local/sbin ./usr/sbin ./sbin 2> /dev/nu
| xargs -L 1 basename | sort | un
| xargs -I{} ln -s "../glaze" "$tmp/root/opt/$name/bin/{}
cd "$tmp/root"
for var in "$@"
do
    mkdir -pm 755 ./usr/local/bin/
    ln -s "/opt/$name/glaze" "./usr/local/bin/$var"
done
cd "$tmp/root"
tar -czf "$outfile" .
