<html>
<head>

<meta charset="TIS-620">
<title>Set On Watch Reason</title>
   
</head>

<body>
<?
$mode = $_REQUEST["mode"];
$entid = $_REQUEST["ENTITYID"];
$login_name = $_REQUEST["LOGINNAME"];
?>

<form name="frm" action="risk_bno_mgr.php" method="POST" >
    <input type="hidden" name="ENTITYID" value="<?=$entid?>">
    <input type="hidden" name="LOGINNAME" value="<?=$login_name?>">
</form>

<script>

    window.open("about:blank", "Risk_Bno", "toolbar=no,titlebar=no,location=no,width=650,height=600,resizable=yes");
    document.frm.target = "Risk_Bno";
    document.frm.submit();
    
</script>

</body>
</html>