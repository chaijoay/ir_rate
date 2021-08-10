<?php require '_web_include/db_util.php'; ?>
<?php require '_web_include/str_util.php'; ?>
<!DOCTYPE html>
<html>
<head>
<!-- http://10.252.55.35/FRM/onwatch_reason.php?UID=5V6qn&PWD=5U~fffffKtpiHn43PnhT0u5%3D%3D&PRT=4294967295&LOGINNAME=5V6qn&LOGINPWD=5U~fffffKtpiHn43PnhT0u5%3D%3D&ENTITYID=101 -->
<!-- http://10.252.55.35/FRM/onwatch_reason.php?LOGINNAME=5V6qn&ENTITYID=102 -->

    <meta charset="TIS-620">
    <title>Risk Bno Management</title>
    <style>
    #textForm {
        border: 1px solid #A4A4A4;
        background-color: #F4F4F4;
        border-radius: 12px;
        width: 600px;
    }

    #resultForm {
        border: 1px solid #A4A4A4;
        border-radius: 12px;
        width: 600px;
    }
    </style>

</head>

<body>

    <?
    error_reporting(0);
    $conn = connectDB();

    $mode = $_REQUEST["mode"];
    $entid = $_REQUEST["ENTITYID"];
    $login_name = decodeStr($_REQUEST["LOGINNAME"]);

    if ( ! checkLogin($login_name, $conn) && $mode != "insert" ) {
        exit;
    }

    $case_no = $_REQUEST["case_number"];
    if ( $case_no == NULL ) {
        $sql = "SELECT CASE_NUMBER FROM CASE_DETAILS WHERE ENTITY_ID = ".$entid;
        $stid = oci_parse($conn, $sql);
        oci_execute($stid, OCI_DEFAULT);
        if ( $row = oci_fetch_array($stid, OCI_ASSOC) ) {
            $case_no = $row['CASE_NUMBER'];
        }
        else {
            echo "<br>unable to get case number<br>";
            exit;
        }
        oci_free_statement($stid);
    }


    if ( $mode == "insert" ) {

        $_conn = new_connect_db();
        try {
            $reason = $_REQUEST["reason"];
            $db_cqs = $_REQUEST["tb_cqs"]*100;
            $db_fms = $_REQUEST["tb_fms"]*100;
            $db_credit = $_REQUEST["tb_credit"]*100;
            $db_maxp = $_REQUEST["tb_maxp"]*100;

            // save original value for displaying as user types when unable to insert/update db
            $tb_cqs = $_REQUEST["tb_cqs"];
            $tb_fms = $_REQUEST["tb_fms"];
            $tb_credit = $_REQUEST["tb_credit"];
            $tb_maxp = $_REQUEST["tb_maxp"];

            $desc = $_REQUEST["desc"];
            $entid = $_REQUEST["entity_id"];
            $login_name = $_REQUEST["login_name"];

            oci_free_statement($stid);
            $sql = "INSERT INTO ONWATCH_PROCESS ".
            "(CASE_NUMBER, ENTITY_ID, RESOLUTION_DATA1, CQS_USAGE, FMS_USAGE, CREDIT_LIMIT, MAX_PAID, DESCRIPTION, STATUS, CREATED_UPDATE, STATUS_UPDATE, USER_UPDATE) ".
            "VALUES (".$case_no.", ".$entid.", ".$reason.", ".$db_cqs.", ".$db_fms.", ".$db_credit.",".$db_maxp.", '".$desc."', 'W', SYSDATE, SYSDATE, SYSDATE)";

            $objParse = oci_parse($_conn, $sql);
            $objExecute = oci_execute($objParse, OCI_DEFAULT);
            if ( $objExecute === true ) {
                //oci_commit($_conn);
            }
            else {
                $e = oci_error($objParse);
                oci_free_statement($objParse);
                $sql = "UPDATE ONWATCH_PROCESS SET USER_UPDATE = SYSDATE, RESOLUTION_DATA1 = ".$reason.
                ", CQS_USAGE = ".$db_cqs.", FMS_USAGE = ".$db_fms.", CREDIT_LIMIT = ".$db_credit.", MAX_PAID = ".$db_maxp.", DESCRIPTION = '".$desc."' ".
                "WHERE CASE_NUMBER = ".$case_no." AND ENTITY_ID = ".$entid;
                $objParse = oci_parse($_conn, $sql);
                $objExecute = oci_execute($objParse, OCI_DEFAULT);
                if ( $objExecute ) {
                    //oci_commit($_conn);
                }
                else {
                    $e = oci_error($objParse);
                    oci_rollback($_conn);
                }
                oci_free_statement($objParse);
            }
            oci_free_statement($objParse);
            $sql = "INSERT INTO ONWATCH_REASON_HIST ".
            "(CASE_NUMBER, RESOLUTION_DATA1, CQS_USAGE, FMS_USAGE, CREDIT_LIMIT, MAX_PAID, DESCRIPTION, LOGIN) ".
            "VALUES(".$case_no.", ".$reason.", ".$db_cqs.", ".$db_fms.", ".$db_credit.", ".$db_maxp.", '".$desc."', '".$login_name."')";
            $objParse = oci_parse($_conn, $sql);
            $objExecute = oci_execute($objParse, OCI_DEFAULT);
            if ( $objExecute ) {
                //oci_commit($_conn);
                $result = "y";
                $reason = ""; $tb_cqs = ""; $tb_fms = "";
                $tb_credit = ""; $tb_maxp = ""; $desc = "";
                // set status = 4 (on watch) for the case number
                $sql = "UPDATE CASE_DETAILS SET STATUS = 4 WHERE CASE_NUMBER = ".$case_no." AND ENTITY_ID = ".$entid;
                $objParse = oci_parse($_conn, $sql);
                $objExecute = oci_execute($objParse, OCI_DEFAULT);
                if ( ! $objExecute ) {
                    $e = oci_error($objParse);
                    oci_rollback($_conn);
                }
                else {
                    // commit all insert/update statements.
                    oci_commit($_conn);
                }
                oci_free_statement($objParse);
            }
            else {
                $e = oci_error($objParse);
                if ( strpos($e['message'], "unique constraint") !== false ) {
                    $result = "Cannot insert : Duplicated Value in History Table";
                }
                else {
                    $result = "Cannot insert : ".htmlentities($e['message']);
                }
                oci_rollback($_conn);
            }
            oci_free_statement($objParse);

        }
        catch ( SQLException $se ) {
            $result = "<font color='red'>".$se->getMessage()."</font>";
        }
        catch ( Exception $e ) {
            $result = "<font color='red'>".$e->getMessage()."</font>";
        }
        
    ?>
        <script>window.close()</script>
    <?
    }
    ?>

    <p><h3>Set On Watch Reason</h3></p>
    <p>select reason and enter usage(in unit of BAHT) for case# <?=$case_no?></p>

    <table align="center" width="400">
        <tr><td>
        <div id="textForm" align="center">
            <br>
            <form id="frm" name="frm" action="onwatch_reason.php" method="post">
                <table align="center" style="font-family:sans-serif" cellpadding="2">
                    <tr>
                        <td align="right">On Watch Reason :</td>
                        <td>
                            <select name="reason">
                                <option value="">Choose Reason</option>
                                <?
                                $sql = "SELECT VALUE, FULL_STRING FROM CODED_VALUE WHERE FIELD_ID = 2024";
                                $stid = oci_parse($conn, $sql);
                                oci_execute($stid, OCI_DEFAULT);
                                while ( ($row = oci_fetch_array($stid, OCI_ASSOC)) != false ) {
                                    echo "<option value='".$row['VALUE']."'>".$row['FULL_STRING']."</option>";
                                }
                                oci_free_statement($stid);
                                ?>
                            </select>
                        </td>
                    </tr>
                    <tr>
                        <td align="right">CQS Usage :</td>
                        <td><input type="text" name="tb_cqs" id="cqs" value="<?=$tb_cqs?>" size="25"></td>
                    </tr>
                    <tr>
                        <td align="right">FMS Usage :</td>
                        <td><input type="text" name="tb_fms" id="fms" value="<?=$tb_fms?>" size="25"></td>
                    </tr>
                    <tr>
                        <td align="right">Credit Limit :</td>
                        <td><input type="text" name="tb_credit" id="credit" value="<?=$tb_credit?>" size="25"></td>
                    </tr>
                    <tr>
                        <td align="right">Max Paid :</td>
                        <td><input type="text" name="tb_maxp" id="maxp" value="<?=$tb_maxp?>" size="25"></td>
                    </tr>
                    <tr>
                        <td align="right">Description :</td>
                        <!-- <td><input type="text" name="desc" id="desc" value="<?//=$desc?>" size="250"></td> -->
                        <td><textarea rows="4" cols="50" name="desc" id="desc"><?=$desc?></textarea></td>
                    </tr>
                    <tr>
                        <td colspan="2" align="center" height="50">
                            <input type="button" id="OnWatch" name="OnWatch" onclick="beforeSubmit();" value="Set On Watch">
                            <input type="hidden" name="mode" value="insert">
                            <input type="hidden" name="entity_id" value="<?=$entid?>">
                            <input type="hidden" name="case_number" value="<?=$case_no?>">
                            <input type="hidden" name="login_name" value="<?=$login_name?>">
                        </td>
                    </tr>
                </table>
            </form>
        </div>
        </tr></td>
    </table>

    <div align="center">

        <?
            if ( $result != "y" ) { // check last insert result to inform user
                 echo "<font color='red'>".$result."</font><br>";
            }
            else {
                echo "<br><br>";
            }

            $sql = "SELECT A.FULL_STRING, B.CQS_USAGE, B.FMS_USAGE, B.CREDIT_LIMIT, B.MAX_PAID, B.DESCRIPTION FROM CODED_VALUE A, ONWATCH_PROCESS B WHERE A.FIELD_ID = 2024 AND B.STATUS = 'W' AND A.VALUE = B.RESOLUTION_DATA1 AND B.ENTITY_ID = ".$entid." AND B.CASE_NUMBER = ".$case_no;
            $stid = oci_parse($conn, $sql);
            oci_execute($stid, OCI_DEFAULT);
            $row = oci_fetch_array($stid, OCI_ASSOC);

            if ( $row ) {
        ?>
            <p><font color='green'>Recent Update Information</font></p>
            <div id="resultForm" align="center">
            <table align="center" style="font-family:sans-serif" cellpadding="4">
                <tr><td colspan="2" align="left">On Watch Reason : <font color='blue'><?=$row['FULL_STRING']?></font></td></tr>
                <tr><td colspan="2" align="left">On Watch Description : <font color='blue'><?=$row['DESCRIPTION']?></font></td></tr>
                <tr>
                    <td width="50%">CQS Usage : <font color='blue'><?=($row['CQS_USAGE']/100)?></font></td>
                    <td>FMS Usage : <font color='blue'><?=($row["FMS_USAGE"]/100)?></font></td>
                </tr>
                <tr>
                    <td>Credit Limit : <font color='blue'><?=($row['CREDIT_LIMIT']/100)?></font></td>
                    <td>Max Paid : <font color='blue'><?=($row['MAX_PAID']/100)?></font></td>
                </tr>
            </table>
            </div>
        <? }
           oci_free_statement($stid);
        ?>
    </div>

<script>
    document.frm.reason.value = "<?=$reason?>";
    function beforeSubmit()
    {
        frm = document.frm;
        if ( frm.reason.value == "" ) {
            alert("Please choose reason");
            frm.reason.focus();
        }
        else if ( frm.cqs.value.match(/^\d+(\.\d\d)?$/) == null ) {
            alert("Please input CQS Usage in currency format ( Baht[.Satang] )");
            frm.cqs.focus();
        }
        else if ( frm.fms.value.match(/^\d+(\.\d\d)?$/) == null ) {
            alert("Please input FMS Usage in currency format ( Baht[.Satang] )");
            frm.fms.focus();
        }
        else if ( frm.credit.value.match(/^\d+(\.\d\d)?$/) == null ) {
            alert("Please input Credit Limit in currency format ( Baht[.Satang] )");
            frm.credit.focus();
        }
        else if ( frm.maxp.value.match(/^\d+(\.\d\d)?$/) == null ) {
            alert("Please input Max Paid in currency format ( Baht[.Satang] )");
            frm.maxp.focus();
        }
        else {
            frm.submit();
        }
    }
</script>

</body>
</html>
<?
oci_close($conn);
oci_close($_conn);
?>
