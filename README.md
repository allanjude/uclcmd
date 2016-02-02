uclcmd
======

Command line tool for working with UCL config files

Designed to be somewhat compatible with [jq](http://stedolan.github.io/jq/)
by implementing a very similar syntax: .object.object.arrayindex|command
instead of jq's .object.object[arrayindex]|command

Although uclcmd commands are not actually 'piped' and only the 'each' command
can be stacked. Running other commands just runs them sequentially

uclcmd currently targets the master branch of [vstakhov/libucl](https://github.com/vstakhov/libucl), as some used features are not available in an existing release.