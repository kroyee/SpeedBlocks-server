<?php
$current_version = $_POST['version'];
$version_info = json_decode(file_get_contents("version_info.json"));
$return = "";

foreach ($version_info as $key => $value) {
	if ($key > $current_version) {
		foreach ($variable as $value) {
			$return .= $value . ';';
		}
	}
}

echo $return;
?>