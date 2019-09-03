# until
Command line runner to timeout dead or long running commands

Usage:
until timeout [-T text] command ...

"timeout": a required integer specifying the number of seconds a process should run for.
-T text: an optional text string to search for in the command output to stop the timeout.

Examples:
until 30 -T "ogin:" ssh user@somehost (Try to ssh to somehost, if it sees the string login:, its fine, otherwise kill after 30 seconds)
until 300 ./myFtpScript.pl (Run myFtpScript.pl, die off if not done in 5 minutes)
