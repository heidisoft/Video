<?php
// $Id: video.drush.inc,v 1.1.4.2 2010/12/05 12:56:19 heshanmw Exp $

/**
 * Implementation of hook_drush_command().
 */
function video_drush_command() {
  $items = array();
  
  $items['video-scheduler'] = array(
    'description' => 'Run video transcoder scheduler',
    'callback' => 'drush_video_scheduler',
    'drupal dependencies' => array('video'),
    'options' => array(
      '--limit' => 'Change the number of video items to transcode',
    ),
  );
  
  return $items;
}

function drush_video_scheduler() {
  $limit = (int) drush_get_option('limit', variable_get('video_ffmpeg_instances', 5));
  $GLOBALS['conf']['video_ffmpeg_instances'] = $limit;

  // include our conversion class (also contains our defines)
  module_load_include('inc', 'video', 'includes/conversion');
  $video_conversion = new video_conversion;
  $video_conversion->run_queue();
}