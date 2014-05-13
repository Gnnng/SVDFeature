'use strict';

module.exports = function(grunt) {
  //Project configuration
  grunt.initConfig({
    jshint: {
      all: [
        'Gruntfile.js',
        '<%= nodeunit.tests %>'
      ],
      options: {
        jshintrc: '.jshintrc'
      }
    },

    // Watch
    watch: {
      all: {
        files: ['<%= jshint.all %>'],
        tasks: ['jshint', 'nodeunit']
      }
    },

    // Unit test
    nodeunit: {
      tests: ['test/**/*_test.js']
    }
  });

  //These plugins provide necessary tasks.
  grunt.loadNpmTasks('grunt-contrib-jshint');
  grunt.loadNpmTasks('grunt-contrib-nodeunit');
  grunt.loadNpmTasks('grunt-contrib-internal');

  // run all test
  grunt.registerTask('default', ['jshint', 'nodeunit']);

};