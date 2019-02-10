<?php
$patch_location = "/var/www/html/update/new/";

function get_file_array($version_info, $current_version, $os) {
	$return = array();
	$breakval = 0;
	$new_version = 0;
	foreach ($version_info as $version => $platform_val) {
		if ($version > $current_version) {
			if ((int)$version > $new_version)
				$new_version = (int)$version;
			foreach ($platform_val as $platform => $file_info) {
				if ($platform == $os OR $platform == "all") {
					foreach ($file_info as $filename => $md5sum) {
						if ($filename == "changelog" AND array_key_exists($filename, $return))
							$return["$filename"] .= "\n\n" . $md5sum;
						elseif ($filename == "halt")
							$breakval = 1;
						else
							$return["$filename"] = $md5sum;
					}
				}
			}
			if ($breakval == 1)
				break;
		}
	}
	$return['new_version'] = $new_version;
	if (empty($return))
		$return['latest'] = 1;
	else
		$return['update'] = 1;

	return $return;
}

function print_patch_info($file_array) {
	foreach ($file_array as $filename => $md5sum)
		echo "$filename;$md5sum;";
}

function make_zip($current_version, $new_version, $os, $file_array) {
	global $patch_location;

	$patch_filename = $patch_location . "$current_version" . "_" . "$new_version" . ".zip";
	if (file_exists($patch_filename))
		return TRUE;

	$zip = new ZipArchive();
	if ($zip->open($patch_filename, ZipArchive::CREATE) !== TRUE)
		return FALSE;

	$patch_location .= $os;

	foreach ($file_array as $filename => $md5sum) {
		if ($filename != 'update' && $filename != 'changelog') {
			$fullpath = $patch_location . $filename;
			echo $fullpath . "\n";
			$zip->addFile($patch_location . $filename, $filename);
		}
	}

	return $zip->close();
}

$current_version = $_POST['version'];
$os = $_POST['os'];
$json = file_get_contents("version_info.json");
$version_info = json_decode($json, true);

$file_array = get_file_array($version_info, $current_version, $os);
$new_version = $file_array['new_version'];
unset($file_array['new_version']);

if (array_key_exists('update', $file_array) && (int)$current_version <= 50) {
	if (make_zip($current_version, $new_version, $os, $file_array)) {
		$file_array = array();
		$patch_filename = "$current_version" . "_" . "$new_version" . ".zip";
		$fullpath = $patch_location . $os . "/" . $patch_filename;
		$file_array['update'] = 1;
		$file_array[$patch_filename] = md5_file($fullpath);
	}
}

print_patch_info($file_array);

?>
