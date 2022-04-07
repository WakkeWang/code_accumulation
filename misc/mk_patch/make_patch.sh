target=install-patch.sh
cp src_patch.sh $target

cd source
tar -czf ../install.tgz bin lib
cd - > /dev/null
 
echo -e "\nPAYLOAD:" >> $target
cat install.tgz >> $target
 
rm -rf install.tgz
