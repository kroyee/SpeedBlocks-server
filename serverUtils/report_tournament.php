<?php
$servername = "localhost";
$username = "-";
$password = "-";
$dbname = "-";

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);
// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

$postdata = json_decode(file_get_contents('php://input'));
if ($postdata->depth == NULL) {
	echo "No depth specified";
	exit();
}
if ($postdata->grade == NULL) {
	echo "No grade specified";
	exit();
}

$reward = 0;
$gradeStr = "";
if ($postdata->grade == 1) {
	$reward = 8000;
	$gradeStr = "gradeA";
}
elseif ($postdata->grade == 2) {
	$reward = 2700;
	$gradeStr = "gradeB";
}
elseif ($postdata->grade == 3) {
	$reward = 650;
	$gradeStr = "gradeC";
}
elseif ($postdata->grade == 4) {
	$reward = 100;
	$gradeStr = "gradeD";
}
else {
	echo "Invalid Grade";
	exit();
}

$modifier = (3-$postdata->depth)*$reward*0.2;
if ($modifier < 0)
	$modifier = 0;

foreach ($postdata as $key => $value) {
	if ($key == "depth" || $key == "grade")
		continue;
	$sql = "UPDATE tstats SET played=played+1,";
	if ($value == 1)
		$sql .= "won=won+1,$gradeStr=$gradeStr+$reward-$modifier WHERE user_id=$key";
	elseif ($value == 2)
		$sql .= "$gradeStr=$gradeStr+$reward*0.7-$modifier WHERE user_id=$key";
	elseif ($value == 3)
		$sql .= "$gradeStr=$gradeStr+$reward*0.4-$modifier WHERE user_id=$key";
	elseif ($value == 4)
		$sql .= "$gradeStr=$gradeStr+$reward*0.2-$modifier WHERE user_id=$key";

	$result = $conn->query($sql);
}

echo "Tournament score reported";

$conn->close();
?>