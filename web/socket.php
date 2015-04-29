<?php

header('Content-Type: application/json');

$VAR_LEN = 20;
$VAL_LEN = 10;
$STR_LEN = 29;
$VARVALSTR = $VAR_LEN + $VAL_LEN + $STR_LEN + 1;
$FORMAT = "%-20s%-10s%-29s\n";

$socket_name = "/tmp/capstone";

if (isset($_POST['Variable']))
	$var = $_POST['Variable'];
else
{
	echo json_encode(
		array(
			"Variable" => "PHP Error",
			"Value" => -1,
			"String" => "No PHP Variable"
		)
	);
	exit;
}
$var = trim(substr($var, 0, $VAR_LEN));

if (isset($_POST['Value']))
	$val = $_POST['Value'];
else
	$val = "0";
$val = trim(substr($val, 0, $VAL_LEN));

//create socket
$socket = socket_create(AF_UNIX, SOCK_STREAM, 0);
if ($socket === FALSE)
{
	echo json_encode(
		array(
			"Variable" => "PHP Error",
			"Value" => -1,
			"String" => "PHP Socket Create Error"
		)
	);
	exit;
}

//bind to server socket
$ret = socket_connect($socket, $socket_name);
if ($ret === FALSE)
{
	echo json_encode(
		array(
			"Variable" => "PHP Error",
			"Value" => -1,
			"String" => "PHP Socket Connect Error"
		)
	);
	socket_close($socket);
	exit;
}

//write variable/value/string to socket
$buffer = sprintf($FORMAT, $var, $val, " ");
//echo "Buffer: ".$buffer."\n";
while (strlen($buffer) > 0)
{
	$ret = socket_write($socket, $buffer, strlen($buffer));
	if ($ret === FALSE)
	{
		echo json_encode(
			array(
				"Variable" => "PHP Error",
				"Value" => -1,
				"String" => "PHP Socket Write Error"
			)
		);
		socket_close($socket);
		exit;
	}
	$buffer = substr($buffer, $ret);
}

//read in variable/value/string
$buffer = "";
while (strlen($buffer) < $VARVALSTR)
{
	$retStr = socket_read($socket, ($VARVALSTR-strlen($buffer)), PHP_BINARY_READ);
	if (($retStr === FALSE) || (strlen($retStr) == 0))
	{
		echo json_encode(
			array(
				"Variable" => "PHP Error",
				"Value" => -1,
				"String" => "PHP Socket Read Error"
			)
		);
		socket_close($socket);
		exit;
	}
	$buffer .= $retStr;
}
//echo "Buffer: ".$buffer."\n";

//need to clip buffer and convert value to numerical
$var = trim(substr($buffer, 0, $VAR_LEN));
//$val = trim(substr($buffer, $VAR_LEN, $VAL_LEN));
$numVal = (float)substr($buffer, $VAR_LEN, $VAL_LEN);
$str = trim(substr($buffer, ($VAR_LEN+$VAL_LEN), $STR_LEN));

socket_close($socket);

//print data
echo json_encode(
	array(
		"Variable" => $var,
		"Value" => $numVal,
		"String" => $str
	)
);

?>
