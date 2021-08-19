# ir_rate

## Reference release program

| app_vers | ver | date        | execute              | Comment
| -------- | --- | ----------- | -------------------- | -------
| 1.0.0    | 1.0 | 26/Feb/2020 | ir_rate.exe_20200226 | First program
| 1.1.0    | 1.1 | 21/Apr/2020 | ir_rate.exe_20200421 | SCP avatar relocate ir common field (cutover 21/04/2020 )
| 1.1.1    | 1.2 | 12/May/2020 | ir_rate.exe_20200512 | Fixed Dialed digit field is null -> change to Connected digit
| 1.1.2    | 1.3 | 07/Jul/2020 | ir_rate.exe_20200707 | Fixed Statement for check prepaid / postpaid ( add status_cd not like 'Disconnect%' )
| 1.2.0    | 1.4 | 09/Jul/2020 | ir_rate.exe_20200709 | Fixed process write charge amount wrong format
| 1.2.1    | 1.5 | 18/Jul/2020 | ir_rate.exe_20200718 | Fixed Invalid Rating ( MTC , SMO )
| 1.2.2    | 1.6 | 18/Aug/2021 | ir_rate.exe          | Fixed wrong Idd Access Code checking (array out of index)
