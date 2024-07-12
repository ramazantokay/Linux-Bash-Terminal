# Custom Linux Bash Terminal in C++

This project is a custom Linux bash terminal program, implemented in C++, which supports the sequential and parallel execution of programs together with a pipeline. 

## Features
- **Pipeline**: Execute commands in a pipeline using the ``|`` delimiter.
- **Sequential Execution**: Execute commands one after another using the ``;`` delimiter.
- **Parallel Execution**: Execute commands simultaneously using the ``,`` delimiter.
- **Subshell Execution**: Group commands into a child shell using parentheses, supporting either sequential or parallel execution.

## Usage

### Pipeline Execution

Commands can be connected using the `|` delimiter, where the output of one command serves as the input to the next.
```bash
ps aux | grep python | wc -l
```
This will count the number of Python processes running.

### Sequential Execution
Commands can be executed sequentially by separating them with a semicolon `;`.

```bash	
echo "Hello"; sleep 5; echo "World"
```

This will print "Hello", wait for 5 seconds, and then print "World".

### Parallel Execution
Commands can be executed in parallel by separating them with a comma `,`.

```bash
echo "Hello", sleep 5, echo "World"
```

All three commands will execute simultaneously.

### Subshell Execution

Commands can be grouped into a subshell using parentheses `()`. The subshell can then be executed sequentially, in parallel, or as a pipeline.

#### Sequential Subshell Example:

```bash
(echo "Hello"; sleep 3; echo "World")
```

#### Parallel Subshell Example:

```bash
(echo "Hello", sleep 2, echo "World")
```
#### Pipeline Subshell Example:

```bash
(echo "Hello" | sleep 2 | echo "World")
```

### Combining Execution Types

Sequential, parallel, and pipeline executions can be combined for more complex command execution flows.

#### Sequential with Pipeline Example:

```bash
echo "Hello" | cat ; cat config.txt; cat manifest.txt | grep "Ship" | tr /a-z/ /A-Z/
```
This will print "Hello", display the contents of config.txt, and then process manifest.txt to search for "Ship" and convert the results to uppercase.

#### Parallel with Pipeline Example:

```bash
echo "Hello" | cat , cat config.txt , cat manifest.txt | grep "Ship" | tr /a-z/ /A-Z/
```
This will execute the commands in parallel with pipelines incorporated.

## Installation

1. Clone the repository:
```bash
git clone https://github.com/ramazantokay/Linux-Bash-Terminal.git
```

2. Navigate to the project directory:
```bash
cd Linux-Bash-Terminal-main/src
```

3. Compile the program:
```bash
make all
```

4. Run the program:
```bash
./eshell
```

## Disclaimer
This project is for educational purposes only. It is provided "as is" without warranty of any kind, express or implied. The authors are not responsible for any damage or data loss that may occur as a result of using this software. Use it at your own risk and always ensure you have appropriate backups and safeguards in place.

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.




