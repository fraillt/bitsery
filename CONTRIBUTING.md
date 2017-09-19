# Contributing to Bitsery

So you want to contribute something to Bitsery? That's great!
Contributions are always very much appreciated, whether it's a bug fix, a new feature or documentation.
However, to make sure the process of accepting patches goes smoothly, you should try to follow these few simple guidelines when
you contribute:

1. Fork the repository.
2. Create new branch based on the *master* branch (`git checkout -b your_branch master`). If your contribution is a bug fix, you should name your branch `bugfix/xxx`; for a feature, it should be `feature/xxx`. Otherwise, just use your good judgment. Consistent naming of branches is appreciated since it makes the output of `git branch` easier to understand with a single glance.
3. Do your modifications on that branch. Except for special cases, your contribution should include proper unit tests and documentation.
4. Make sure your modifications did not break anything by building, running tests and checking code coverage (test coverage should not be less than 100%):
  ```shell
  mkdir build
  cd build
  cmake ..
  make
  ctest
  make tests_coverage
  x-www-browser ./coverage/index.html
  ```
5. Commit your changes, and push to your fork (`git push origin your_branch`). Commit message should be one line short description. When applicable, please squash adjacent *wip* commits into a single *logical* commit.
6. Open a pull request against Bitsery *master* branch. Currently ongoing development is on *master*. At some point an integration branch will be set-up, and pull-requests should target that, but for now its all against master. You may see feature branches come and go, too.


## Style guide

Just use your own judgment and stick to the style of the surrounding code.