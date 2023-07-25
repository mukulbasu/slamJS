# The data folder is used for accessing test data. Download test data in this directory
# "data1" directory is reserved for uploading new content. 
# Everytime you want to upload new set of test data, please restart the server 
# so that the old data is deleted and not mixed with the new one.
mkdir ../data
mkdir ../data/data1
rm -f ../data/data1/*

yarn install
node server.js