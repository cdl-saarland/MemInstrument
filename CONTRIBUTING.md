# Contributing to MemInstrument

Thank you for your interest to contribute to MemInstrument!

## Project goals

This project is developed as part of PhD Theses.
As such, we cannot guarantee that it will be extended by large features in the future.
However, we plan to maintain the project as a hobby.
This means we plan to fix bugs, update to newer LLVM versions and the like.
We are happy to get notifications about issues with the project, bug reports, ideas for improvements etc., as this code base should ideally remain useful for research purposes or other uses in the future.
Due to this being a side project, the response time might vary depending on our main-job workload, and we cannot guarantee that issues will be resolved in a timely manner.

## Questions about the project

If you have questions about the project, feel free to contact the developers via email.
Please do not open issues for questions.
We will start writing a FAQ section in a structured form when we receive questions that we consider interesting to a broad audience.

## Getting started with the framework

If you want to understand how the provided memory safety instrumentations and the framework work, we recommend looking at the [tests](https://github.com/cdl-saarland/MemInstrument/tree/main/test).
See below how to [execute the test](#testing).

As you are testing a C compiler with an extension, test cases are typically small C programs.
The `RUN` lines in the test demonstrate the usage of the different instrumentations as well as some features or limitations of them.
Try playing around with the tests or adding your own tests.
If you put your test in the [tests folder](https://github.com/cdl-saarland/MemInstrument/tree/main/test) and get inspiration from the `RUN` / `CHECK` lines in the existing tests, your new test will automatically be executed with the existing tests.

## Testing

After you successfully [set up the project](https://github.com/cdl-saarland/MemInstrument/blob/main/README.md), you can execute the test by navigating to your LLVM build and running

    ninja check-meminstrument

## Issues

If you encounter a bug or issues, use the GitHub issue mechanism to report it.

## Critical bugs

First off, we cannot give any guarantees on the correctness of the code.
In case the project grows and is used for sensitive applications, we will introduce a responsible disclosure mechanism.
Until then, critical bugs can simply be reported via issues just as non-critical bugs.

## Feature requests

As mentioned in the [Project goals](#project-goals), we do not plan to implement new features.
However, feel free to open the feature request as `enhancement`-tagged issue, and if time permits, we might work on it.

## Contributing code

If you are interested in resolving issues, feel free to implement a solution and create a pull request.
