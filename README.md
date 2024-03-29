
# ECScanner

A tool for reading EC ram. This is actually the first tool of its kind (e.g. `nbfc-linux/ec-probe`) that is able to associate EC ram values with their corresponding field names described in the ACPI table.


## Limitation

1. This tool is only useful when the EC ram is mapped to system memory. That is, in your ACPI table, something similar to the below should appear in the section for device PNP0C09:
    ```
    OperationRegion (ECF2, SystemMemory, 0xFE0B0400, 0xFF)
    Field (ECF2, ByteAcc, Lock, Preserve)
    {
        VCMD,   8, 
        VDAT,   8, 
        ...
    }
    ```

    If there isn't one, consider `nbfc-linux/ec-probe` which communicates with EC ram in a more standard manner through IO ports.

2. EC ram monitoring is not implemented. 

## Reference

- How to properly map `/dev/mem`: [devmem2](https://github.com/radii/devmem2)
