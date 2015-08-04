**ndmq** &ndash; NDM Query utility

SYNOPSIS
--------

    ndmq [OPTION]... -p REQUEST

DESCRIPTION
-----------

The *ndmq* (NDM Query) utility sends a PARSE request to NDM and prints
an XML response to stdout. Path filter can be used to print only desired 
subset of response elements. NDM error code is returned as an exit code 
of *ndmq*.

OPTIONS
-------

-p *REQUEST*  
    Send a PARSE request. *REQUEST* is a plain text command, the same as
    CLI command of the NDM shell (*ndmc*). The maximum size of *REQUEST*
    is 4096 characters.

-A *AGENT*  
    Agent name, defaults to `"ndmq/ci"`. The maximum size of *AGENT* is 64
    characters.

-P *XPATH*  
    Response pattern, that is used to narrow down the subset of printed
    elements. Path syntax is limited to element names only. For example,
    `-P "interface/id"` will show only the values of `<id>` children of
    `<interface>` elements. *XPATH* defaults to `"/"`. The maximum size of
    *XPATH* is 256 characters.

-x  
    Print nested XML nodes. Otherwise, only plain values are printed
    according to *XPATH*.

-h  
    Display short help and exit.

-v  
    Display version information and exit.

EXIT CODE
---------

Successful query returns `EXIT_SUCCESS`.

If NDM is unavailable, or returns an invalid response, or any system
failure is encountered, the `EXIT_FAILURE` is returned.

If NDM responds with an `<error>` or `<critical>` message, the lower 16
bits of the `code` are returned as an exit code of *ndmc*.

The *ndmq* source code and documentation can be downloaded from
<https://github.com/ndmsystems/ndmq/>

