install_path=__prefix__
ln -sf $install_path/lib/*.so /mnt/persistent/rpcmodules/
killall rpcsvc
su -c 'rpcsvc' - gemuser
