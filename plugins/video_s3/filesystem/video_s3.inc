<?php

//$Id: video_s3.inc,v 1.1.4.3 2011/01/08 05:44:36 heshanmw Exp $
/*
 * @file
 * Class file used to store filesystem on the video.
 *
 */
class video_s3 extends video_filesystem {

  protected $params = array();
  protected $name = 'Amazon S3';
  protected $value = 'video_s3';

  public function __construct() {

  }

  public function save_file($video) {

  }

  public function prepare_file($video) {

  }

  public function load_file(&$video) {
    video_s3_video_load(&$video);
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/filesystem_interface#get_name()
   */
  public function get_name() {
    return $this->name;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/filesystem_interface#get_help()
   */
  public function get_help() {
    $help = t('Amazon Simple Storage Service (!s3) to store your video files. This free\'s up bandwidth from your site, providing a faster experience for your users. Simply enable this and enter your authentication details and your done! ', array('!s3' => l(t('Aamzon S3'), 'http://s3.amazonaws.com')));
    return $help;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/filesystem_interface#get_value()
   */
  public function get_value() {
    return $this->value;
  }

  public function run_command($options) {
    return;
  }

  public function admin_settings() {
    $form = array();
    $form['amazon_s3_ssl'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enable SSL?'),
      '#default_value' => variable_get('amazon_s3_ssl', FALSE),
      '#description' => t('If you would like to use ssl when transfering your files enable this option.'),
    );
    $form['amazon_s3_private'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enable Private?'),
      '#default_value' => variable_get('amazon_s3_private', FALSE),
      '#description' => t('If you would like to use private transfering for your files enable this option.'),
    );
    $form['amazon_s3_lifetime'] = array(
      '#type' => 'textfield',
      '#title' => t('Private Url Lifetime'),
      '#default_value' => variable_get('amazon_s3_lifetime', '1800'),
      '#size' => 5,
    );


    $form['amazon_s3_access_key'] = array(
      '#type' => 'textfield',
      '#title' => t('Access Key ID'),
      '#default_value' => variable_get('amazon_s3_access_key', ''),
      '#size' => 50,
    );
    $form['amazon_s3_secret_access_key'] = array(
      '#type' => 'password',
      '#title' => t('Secret Access Key'),
      '#default_value' => variable_get('amazon_s3_secret_access_key', ''),
      '#description' => t('Once saved, you do not need to re-enter your secret key.  If you need to update your key, then fill this out to update it.'),
      '#size' => 50,
    );
    //@todo Maybe move this to the admin settings page instead of global?
    $form['amazon_s3_bucket'] = array(
      '#type' => 'textfield',
      '#title' => t('Bucket'),
      '#description' => t('Enter the bucket you wish to store your videos in.  If the bucket doesn\'t exist the system will attempt to create it.'),
      '#default_value' => variable_get('amazon_s3_bucket', ''),
      '#size' => 50,
    );

    //lets show our buckets in table format with a delete link.
    //@todo add permissions
    //were enabled, that means they have successfully connected and created a bucket.
    if (variable_get('amazon_s3_access_key', false) && variable_get('video_filesystem', 'drupal') == 'video_s3') {
      module_load_include('inc', 'video_s3', '/includes/amazon_s3');
      $s3 = new video_amazon_s3;
      $s3->connect();
      $buckets = $s3->s3->listBuckets();
      // Setup our header.
      $header = array(t('Bucket Name'), t('Total Objects'), t('Actions'));
      $rows = array();
      foreach ($buckets as $bucket) {
        $objects = count($s3->s3->getBucket($bucket));
        $actions = l(t('Delete'), 'admin/settings/video/amazon_s3/bucket/' . $bucket . '/delete');
        $rows[] = array($bucket, $objects, $actions);
      }
      $form['amazon_info'] = array(
        '#type' => 'fieldset',
        '#title' => t('Amazon S3 Information'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
      );
      $form['amazon_info']['buckets'] = array(
        '#type' => 'markup',
        '#value' => theme('table', $header, $rows),
      );
    }
    return $form;
  }

  public function admin_settings_validate(&$form, &$form_state) {
    // Check for CURL
    if (!extension_loaded('curl') && !@dl(PHP_SHLIB_SUFFIX == 'so' ? 'curl.so' : 'php_curl.dll')) {
      form_set_error('amazon_s3', t('The CURL extension is not loaded.'));
    } else {
      $bucket = $form_state['values']['amazon_s3_bucket'];
      // S3 buckets must contain only lower case alphanumeric characters, dots and dashes.
      if (!preg_match("/^[a-z.-]+$/", $bucket)) {
        form_set_error('amazon_s3_bucket', t('S3 buckets must contain only lower case alphanumeric characters, dots and dashes.'));
      } else {
        $access_key = $form_state['values']['amazon_s3_access_key'];
        // check our secret key.
        if (!empty($form_state['values']['amazon_s3_secret_access_key'])) {
          $secret_key = $form_state['values']['amazon_s3_secret_access_key'];
        } else {
          // Add our secret key back in to persist its value.
          $form_state['values']['amazon_s3_secret_access_key'] = variable_get('amazon_s3_secret_access_key', '');
          $secret_key = variable_get('amazon_s3_secret_access_key', '');
        }
        $ssl = isset($form_state['values']['amazon_s3_ssl']) && $form_state['values']['amazon_s3_ssl'] ? TRUE : FALSE;
        // Lets verify our credentials and verify our bucket exists, if not attempt to create it.
        module_load_include('inc', 'video_s3', '/includes/amazon_s3');
        $s3 = new video_amazon_s3;
        $s3->connect($access_key, $secret_key, $ssl);
        $buckets = $s3->s3->listBuckets();
        if (!$buckets || !(in_array($bucket, $buckets))) {
          // Create a bucket with public read access
          if ($s3->s3->putBucket($bucket, S3::ACL_PUBLIC_READ)) {
            // set access control policy to zencoder if module is enabled
            // @TODO : Add this to video_zencoder module
            if (module_exists('video_zencoder')) {
              $acp['acl'][] = array(
                'type' => 'AmazonCustomerByEmail',
                'email' => 'aws@zencoder.com',
                'permission' => 'WRITE'
              );
              $s3->s3->setAccessControlPolicy($bucket, '', $acp);
            }
            drupal_set_message(t('Successfully created the bucket %bucket.', array('%bucket' => $bucket)));
          } else {
            form_set_error('amazon_s3_bucket', t('Could not verify or create the bucket %bucket.', array('%bucket' => $bucket)));
          }
        }
      }
    }
  }

}

?>
