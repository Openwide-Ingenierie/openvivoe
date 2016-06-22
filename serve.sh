#! /bin/zsh

# A script to serve the website "locally" thanks to Jekyll
# on localhost:4000

if which bundle > /dev/null; then
	bundle exec jekyll serve
else
	export PATH=/usr/local/sbin:/usr/local/bin:/usr/bin:/usr/lib/jvm/default/bin:/usr/bin/site_perl:/usr/bin/vendor_perl:/usr/bin/core_perl:$HOME/.gem/ruby/2.3.0/bin
	bundle install
	bundle exec jekyll serve
fi
