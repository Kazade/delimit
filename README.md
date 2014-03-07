# Delimit Text Editor

Delimit is a text editor that I'm building for *my* usage pattern. It won't support a plugin infrastructure, or try to do everything - just the things I care about.

## Compiling and running

From the root of the project:

 - mkdir build
 - cd build
 - cmake ..
 - make 
 - sudo make install

##TODO

 - Make split view work
 - Implement a code completion framework, initially targetting Python
 - Implement basic VCS integration, initially targetting GIT (show current branch, show add/deletes in gutter, blame)
 - Implement a checker framework, initially using pyflakes
 - Implement a contextual menu for the folder/file tree with open, rename, delete or new file, new subfolder, rename, delete depending on the type
 - Add close buttons to the open document pane
 - Django detection and support (Django specific code completions, management command buttons)
 - ST style file/class/function location popup (depends on CC)
 - Watch the folder tree for changes (e.g. folder/file deletions/renames/creations)
 - Distinguish between open files of the same name in the open files section
 
## Submitting patches

Because I'm still not convinced LGPL is the right license for this, if you submit patches please submit them under the MIT license. Thanks!
