install_path=__prefix__
for i in $(ls $install_path/lib/); do
    ln -sf $install_path/lib/$i /mnt/persistent/rpcmodules/$i
done
