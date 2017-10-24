## How to contribute to NeuroDataManager

#### **Did you find a bug?**

* We use GitHub for issue tracking. Simply submit an issue on our [Issues](https://github.com/neurodata/DataManager/issues) page.

* If you want to take a crack at the issue, feel free to let us know. While we strive to always report issues in GitHub, sometimes issues raised internally may slip through the cracks. Letting us know before you dig into the issue avoids duplication of effort.

* Pull requests for issues are always welcome. If it's a quick fix, feel free to open a PR in lieu of an issue.

* We will provide a level of support commensurate with your level of effort to describe the issue. No issues forms here, but generally we suggest you provide the following:
  * Command(s) used 
  * Dataset information (a Neuroglancer manifest file is always helpful, even without data)
  * NDM Version and build date (`bin/ndm -version`)
 
  Of course, all of that information is not always required.
  
#### **Did you fix whitespace, format code, or make a purely cosmetic patch?**

Changes that are cosmetic in nature and do not add anything substantial to the stability, functionality, or testability of NeuroDataManager will generally not be accepted. When in doubt, ask first!

#### **Do you intend to add a new feature or change an existing one?**

* Open an issue or contact one of the repository maintainers. We're always receptive to discussing new features. Note that we designed NeuroDataManager to be as lightweight and flexible as possible. Thus, some design choices were made that limited the feature set in favor of simplicity and/or performance. If your feature request will add considerable complexity to the code base, it might be more appropriate to fork NeuroDataManager and link the appropriate libraries into your application. 

* Make sure to format your code using our `clang-format` file before finalizing your PR. Good practice is to run `clang-format -i <<filename>>` before each commit (or before each rebase).

* **No merge commits!** Please rebase your branch on top of the master branch as you develop. The following example illustrates a rebase:

  * `git checkout -b yourbranch`
  * **add your feature** 
  * `git commit -a 'Add your awesome new feature'`
  * `git checkout master`
  * `git pull`
  * `git checkout yourbranch`
  * `git rebase master`
  
* We will rebase your commits on top of master when we accept your PR. Thus, we ask that you squash your commits into as few commits as possible before sending a pull request. More than one commit per PR is certainly acceptable. However, each commit should describe a logically separate feature/fix/addition to the codebase. 
  
  The following history is acceptable:
  * Add awesome feature x to ingest 
  * Add awesome feature y to mesh
  
  The following history should be rebased before PR:
  * Add awesome feature x to ingest
  * Fix bug in awesome feature x 


* Following these guidelines helps keep our commit history clean and allow many people to use the same code base harmoniously. If you have any questions, feel free to open a PR and ask! You can always force push to update your PR. 

Based on CONTRIBUTING.md from [Ruby on Rails](https://github.com/rails/rails).
