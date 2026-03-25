module.exports = {
  dependency: {
    platforms: {
      ios: {
        podspecPath: __dirname + '/Geomony.podspec',
      },
      android: {
        sourceDir: __dirname + '/android',
        cmakeListsPath: __dirname + '/cpp/CMakeLists.txt',
      },
    },
  },
};
