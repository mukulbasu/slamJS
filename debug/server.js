/**
 * @file server.js
 * @brief This runs a local server for debugging and testing SLAM on web.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
const http = require('http');
const fs = require('fs');
const path = require("path");
const formidable = require('formidable');

// html file containing upload form

// replace this with the location to save uploaded files
var upload_path = "../data/data1/";
const fileCache= {};
const fileCacheTs = {};

http.createServer(function (req, res) {
    const headers = {
      'Access-Control-Allow-Origin': '*', /* @dev First, read about security */
      'Access-Control-Allow-Methods': 'OPTIONS, POST, GET',
      'Access-Control-Max-Age': 2592000, // 30 days
      /** add other headers as per requirement */
    };
    if (req.url == '/clearUploadDir') {
      fs.readdir(upload_path, (err, files) => {
        if (err) throw err;
      
        for (const file of files) {
          fs.unlink(path.join(upload_path, file), (err) => {
            if (err) throw err;
          });
        }
      });
      res.writeHead(200, headers);
      return res.end();
    } else if (req.url == '/uploadForm') {
        try {
            var form = new formidable.IncomingForm();
            form.parse(req, function (err, fields, files) {
                // oldpath : temporary folder to which file is saved to
                try {
                    // console.log("Fields", fields);
                    // console.log("Files", files);
                    var oldpath = files.filetoupload.filepath;
                    var newpath = upload_path + files.filetoupload.originalFilename;
                    console.log("Uploading file ", newpath);
                    // copy the file to a new location
                    fs.rename(oldpath, newpath, function (err) {
                        if (err) throw err;
                        // you may respond with another html page
                        var upload_html = fs.readFileSync("uploadFile.html");
                        res.writeHead(200, headers);
                        res.write(upload_html);
                        // res.write('File uploaded and moved!');
                        res.end();
                    });
                } catch (err) {
                    console.error(err);
                    var upload_html = fs.readFileSync("uploadFile.html");
                    res.writeHead(200, headers);
                    res.write(upload_html);
                    return res.end();   
                }
            });
        } catch (err) {
            console.error(err);
            return res.end();
        }
	  } else if (req.url.startsWith('/data/')) {
        res.writeHead(200, headers);
        const fileName = ".." + req.url;
        try {
          var upload_html = fs.readFileSync(fileName);
          res.write(upload_html);
          return res.end();
        } catch (err) {
          console.log(err);
          return res.end();
        }
    } else if (req.url.startsWith('/debug/logs')) {
        res.writeHead(200, headers);
        const fileName = "logs/"+req.url.split("/")[3]+".txt";
        const search = req.url.split("/")[4];
        console.log("FN is ", fileName);
        console.log("Search is ", search);
        try {
          const mtime = fs.statSync(fileName).mtimeMs;
          if (!(fileName in fileCache) || mtime !== fileCacheTs[fileName]) {
            console.log("Updating file cache", fileName, mtime == fileCacheTs[fileName]);
            fileCacheTs[fileName] = mtime;
            fileCache[fileName] = fs.readFileSync(fileName, 'utf8');
          }
          const data = fileCache[fileName];
          const lines = data.split(/\r?\n/).filter((value) => value.match(search));
          // console.log("Lines ", lines);
          const json = {search, lines};
          const resp = JSON.stringify(json);
          return res.end(resp);
        } catch (err) {
          console.log(err);
          return res.end();
        }
    } else if (req.url.startsWith('/cout')) {
      res.writeHead(200, headers);
      const fileName = "tmp";
      const reqParts = req.url.split("/");
      let searchString = "";
      for (let i = 2; i < reqParts.length; i++) {
        searchString = searchString + reqParts[i] + ":";
      }
      console.log("FN is ", fileName);
      console.log("Search is ", searchString);
      try {
        const mtime = fs.statSync(fileName).mtimeMs;
        if (!(fileName in fileCache) || mtime !== fileCacheTs[fileName]) {
          console.log("Updating file cache", fileName, mtime == fileCacheTs[fileName]);
          fileCacheTs[fileName] = mtime;
          fileCache[fileName] = fs.readFileSync(fileName, 'utf8');
        }
        const data = fileCache[fileName];
        const lines = data.split(/\r?\n/);
        let startIndex = -1, endIndex = -1, resultLines = [];
        const searchStart = `${searchString}LOG_START`;
        const searchEnd = `${searchString}LOG_END`;
        console.log("Search ", searchStart, searchEnd);
        for (let i = 0; i< lines.length; i++) {
          if (startIndex == -1) {
            if (lines[i].startsWith(searchStart)) {
              startIndex = i;
              resultLines.push(lines[i]);
            }
          } else {
            if (endIndex == -1) {
              if (lines[i].startsWith(searchEnd)) { 
                resultLines.push(lines[i]);
                endIndex = i; break;
              }
            }
            resultLines.push(lines[i]);
          }
        }
        console.log("Lines ", resultLines);
        const json = {lines: resultLines};
        const resp = JSON.stringify(json);
        return res.end(resp);
      } catch (err) {
        console.log(err);
        return res.end();
      }
  } else {
      const fileName = "."+req.url;
      try {
        var upload_html = fs.readFileSync(fileName);
        if (req.url.includes(".wasm")) res.setHeader('Content-Type', 'application/wasm');
        if (req.url.includes(".js")) res.setHeader('Content-Type', 'text/javascript');
        res.writeHead(200, headers);
        res.write(upload_html);
        return res.end();
      } catch (err) {
        console.log(err);
        res.writeHead(200, headers);
        return res.end();
      }
	}
}).listen(8080);
console.log("Server started");
