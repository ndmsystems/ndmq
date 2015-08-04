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

 __-p _REQUEST___  
    Send a PARSE request. *REQUEST* is a plain text command, the same as
    CLI command of the NDM shell (*ndmc*). The maximum size of *REQUEST*
    is 4096 characters.

 __-A _AGENT___  
    Agent name, defaults to `"ndmq/ci"`. The maximum size of *AGENT* is 64
    characters.

 __-P _XPATH___  
    Response pattern, that is used to narrow down the subset of printed
    elements. Path syntax is limited to element names only. For example,
    `-P "interface/id"` will show only the values of `<id>` children of
    `<interface>` elements. *XPATH* defaults to `"/"`. The maximum size of
    *XPATH* is 256 characters.

 __-x__  
    Print nested XML nodes. Otherwise, only plain values are printed
    according to *XPATH*.

 __-h__  
    Display short help and exit.

 __-v__  
    Display version information and exit.

EXIT CODE
---------

Successful query returns `EXIT_SUCCESS`.

If NDM is unavailable, or returns an invalid response, or any system
failure is encountered, the `EXIT_FAILURE` is returned.

If NDM responds with an `<error>` or `<critical>` message, the lower 16
bits of the `code` are returned as an exit code of *ndmc*.

EXAMPLES
--------

#### Show NDM version info (XML)
```sh   
~ $ ndmq -p "show version" -x
```
```xml
<response>
    <release>v2.06(AAUS.1)A2</release>
    <arch>mips</arch>
    <ndm>
        <exact>0-9c58c99</exact>
        <cdate>4 Aug 2015</cdate>
    </ndm>
    <bsp>
        <exact>0-6c947fe</exact>
        <cdate>4 Aug 2015</cdate>
    </bsp>
    <manufacturer>ZyXEL</manufacturer>
    <vendor>ZyXEL</vendor>
    <series>Keenetic series</series>
    <model>Keenetic</model>
    <hw_version>12132000-F</hw_version>
    <hw_id>kn_rf</hw_id>
    <device>Keenetic Omni II</device>
    <class>Internet Center</class>
    <prompt>(config)</prompt>
</response>
```

#### Show NDM release (XML)
```sh   
~ $ ndmq -p "show version" -P release -x
```
```xml
<release>v2.06(AAUS.1)A2</release>
```

#### Show NDM release (plain)
```sh   
~ $ ndmq -p "show version" -P release
v2.06(AAUS.1)A2
```

#### Show network interfaces
```sh
~ $ ndmq -p "show interface" -P interface/id
FastEthernet0
FastEthernet0/0
FastEthernet0/1
FastEthernet0/2
FastEthernet0/3
FastEthernet0/4
FastEthernet0/Vlan1
FastEthernet0/Vlan2
WifiMaster0
WifiMaster0/AccessPoint0
WifiMaster0/AccessPoint1
WifiMaster0/AccessPoint2
WifiMaster0/AccessPoint3
WifiMaster0/WifiStation0
Bridge0
Bridge1
```

SEE ALSO
--------

The *ndmq* source code and documentation can be downloaded from
<https://github.com/ndmsystems/ndmq/>
