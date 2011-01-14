require 'rubygems'
require 'rake'

require 'jeweler'
Jeweler::Tasks.new do |gem|
  # gem is a Gem::Specification... see http://docs.rubygems.org/read/chapter/20 for more options
  gem.name = "bencode_ext"
  gem.homepage = "http://github.com/naquad/bencode_ext"
  gem.license = "MIT"
  gem.summary = %Q{BitTorrent encoding parser/writer}
  gem.description = %Q{BEncodeExt is implementation of Bencode reader/writer (BitTorent encoding) in C.}
  gem.email = "naquad@gmail.com"
  gem.authors = ["naquad"]
  gem.required_ruby_version = '~>1.9.2'
  gem.add_dependency 'rake-compiler', '~>0.7.5'
  gem.add_development_dependency "jeweler", "~> 1.5.2"
end
Jeweler::RubygemsDotOrgTasks.new
Jeweler::GemcutterTasks.new

require 'rake/testtask'
Rake::TestTask.new(:test) do |test|
  test.libs << 'lib' << 'test'
  test.pattern = 'test/**/test_*.rb'
  test.verbose = true
end

task :default => :test

require 'rake/rdoctask'
Rake::RDocTask.new do |rdoc|
  version = File.exist?('VERSION') ? File.read('VERSION') : ""

  rdoc.rdoc_dir = 'rdoc'
  rdoc.title = "bencode_ext #{version}"
  rdoc.rdoc_files.include('README*')
  rdoc.rdoc_files.include('ext/**/*.c')
end


require 'rake/extensiontask'
Rake::ExtensionTask.new('bencode_ext')
