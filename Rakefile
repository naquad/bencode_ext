require 'rubygems'
require 'rake'

NAME = 'bencode_ext'

desc 'Build gem'
task :build do
  Dir.chdir("ext/bencode_ext") do
    ruby 'extconf.rb'
    sh 'make'
  end

  cp 'ext/bencode_ext/bencode_ext.so', 'lib/bencode_ext.so'
end

require 'rake/testtask'
Rake::TestTask.new(:test) do |test|
  test.libs << 'lib' << 'test'
  test.pattern = 'test/**/test_*.rb'
  test.verbose = true
end

task :default => :test

require 'rdoc/task'
Rake::RDocTask.new do |rdoc|
  version = File.exist?('VERSION') ? File.read('VERSION') : ""

  rdoc.rdoc_dir = 'rdoc'
  rdoc.title = "bencode_ext #{version}"
  rdoc.rdoc_files.include('README*')
  rdoc.rdoc_files.include('ext/**/*.c')
end
