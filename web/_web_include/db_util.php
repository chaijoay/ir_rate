<?php

$ora_home = "/appl/oracle/product/12.1.0";
$dbuser = "cfms";
$dbpwd  = "Maruko8365";
$dbsid  = "//10.252.55.35/FRM";
putenv("ORACLE_HOME=".$ora_home);
putenv("LD_LIBRARY_PATH=".$ora_home."/lib");
putenv("NLS_LANG=AMERICAN_AMERICA.TH8TISASCII");

function new_connect_db()
{
    global $dbuser, $dbpwd, $dbsid;

    $new_conn = oci_new_connect($dbuser, $dbpwd, $dbsid);
    if ( ! $new_conn ) {
        $m = oci_error();
        echo $m['message']."<br>";
    }
    else {
        return $new_conn;
    }
}

function connectDB()
{
    global $dbuser, $dbpwd, $dbsid;    

    $conn = oci_connect($dbuser, $dbpwd, $dbsid);
    if ( !$conn ) {
        $m = oci_error();
        echo $m['message']."<br>";
    }
    else {
        return $conn;
    }
}

function checkLogin($login_name, $conn)
{
    $result = false;
    if ( empty($login_name) || empty($conn)) {
        return $result;
    }
    $sql = "SELECT LOGIN_NAME FROM SYSTEM_USER WHERE LOGIN_NAME = '".$login_name."'";
    $stid = oci_parse($conn, $sql);
    oci_execute($stid, OCI_DEFAULT);
    if ( $row = oci_fetch_array($stid, OCI_ASSOC) ) {
        $login = $row['LOGIN_NAME'];
        if ( $login == $login_name ) {
            $result = true;
        }
    }
    oci_free_statement($stid);
    return $result;
}

?>
