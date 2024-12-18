# # create firmware directory
# mkdir -p firmware
# # create firmware sub directory
# for dir in $(sed -n 's|^[^#]*/path/to/xia-daq-gui-online/firmware/\([^/]*\)/.*lst|\1|gp' parset/cfgPixie16.origin.txt)
# do
# 	mkdir -p "firmware/$dir"
# done

# # download from PKUXIADAQ repository
# for filename in $(sed -n 's|^[^#]*/path/to/xia-daq-gui-online/\(.*$\)|\1|gp' parset/cfgPixie16.origin.txt)
# do
# 	wget https://raw.githubusercontent.com/wuhongyi/PKUXIADAQ/8694bdb2076626ea9642256e71f350c6cadf2b52/$filename -O $filename
# done

# edit
sed -i "s|\(^[^# ]* \).*/xia-daq-gui-online\(/firmware/.*$\)|\1$(pwd)\2|g" parset/cfgPixie16.txt