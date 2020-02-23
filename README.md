# Windows Registry Wrapper

Windows Registry Wrapper aims to streamline the use of the Windows Registry by providing easy to use CRUD functionality.
  - It comes packed as a header-only file, meaning the only dependency you will have to worry about is the Windows API, which this wrapper builds upon.
  - Written in modern C++17

## Functions
The following table illustrates what functions are available for each type of operation
<table>
    <thead>
        <tr>
            <th>Operation</th>
            <th>Function</th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td rowspan=3>Create</td>
            <td>Key</td>
            <td>Creates a key node under one of the hives</td>
        </tr>
        <tr>
            <td>Number</td>
            <td>Creates a value that stores a DWORD under a key</td>
        </tr>
        <tr>
            <td>String</td>
            <td>Creates a value that stores a string under a key</td>
        </tr>
        <tr>
            <td rowspan=5>Query</td>
            <td>Number</td>
            <td>Gets the data stored in a DWORD value</td>
        </tr>
        <tr>
            <td>String</td>
            <td>Gets the data stored in a string value</td>
        </tr>
        <tr>
            <td>Key Info</td>
            <td>Gets number of subkeys, the lenght of the longest subkey, number of values, length of the longest value</td>
        </tr>
        <tr>
            <td>Keys</td>
            <td>Gets names of all subkeys</td>
        </tr>
        <tr>
            <td>Value Names</td>
            <td>Gets names of all values belonging to the key</td>
        </tr>
        <tr>
            <td rowspan=2>Update</td>
            <td>Number</td>
            <td>Sets data for a DWORD value</td>
        </tr>
        <tr>
            <td>String</td>
            <td>Sets data for a string value</td>
        </tr>
        <tr>
            <td rowspan=5>Remove</td>
            <td>Key</td>
            <td>Removes key if it has no children</td>
        </tr>
        <tr>
            <td>Value</td>
            <td>Removes a specific value from a key</td>
        </tr>
        <tr>
            <td>Subkeys</td>
            <td>Removes all direct child nodes</td>
        </tr>
        <tr>
            <td>Values</td>
            <td>Removes all of the key's values</td>
        </tr>
        <tr>
            <td>Cluster</td>
            <td>Removes a key recursively</td>
        </tr>
        <tr>
            <td rowspan=4>Misc</td>
            <td>Peek Value</td>
            <td>Retrieves the type and size of a value</td>
        </tr>
        <tr>
            <td>Key Exists</td>
            <td>Checks if a key exists</td>
        </tr>
        <tr>
            <td>Value Exists</td>
            <td>Checks if a value exists</td>
        </tr>
        <tr>
            <td>Open</td>
            <td>Opens a registry key with the desired access rights</td>
        </tr>
    </tbody>
</table>

An example of how to effectively use these functions is provided in `example.cpp`.

The documentation can be found inside the header file.