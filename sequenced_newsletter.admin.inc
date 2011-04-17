<?php 
// Id

/**
 * @file
 * Administration pages for sequenced newsletter.
 */

/**
 * Administration pages form callback.
 */
function sequenced_newsletter_admin() {
  $form = array();
  $form['add_link'] = array(
    '#value' => theme_item_list( array(
            l('Create sequenced newsletter', 'admin/settings/sequenced_newsletter/add'),
            l('View list of sent sequenced newsletters', 'admin/settings/sequenced_newsletter/list'),
    )),
  );
  
  $form['table'] = array(
    '#value' => theme_table(
                    array(
                      t('Sequenced newsletter name'),
                      t('Actions'),
                    ),
                    _sequenced_newsletter_get_active_items()
                ),
  );
  

  return $form;
}


/**
 * Form callback for sequenced newsletter edit form.
 */
function sequenced_newsletter_admin_edit(&$form_state, $id = -1) {
  // Go back if cancelled
  if (isset($form_state['post']) && isset($form_state['post']['cancel'])) {
    drupal_goto('admin/settings/sequenced_newsletter');
  }
  
  // Load old data if editing existing entry.
  $old_data = array();
  if ($id > -1) {
    $old_data = _sequenced_newsletter_get_item($id);
  }
  
  // Build form.
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t("Sequenced's newsletter name"),
    '#default_value' => isset($old_data['name']) ? check_plain($old_data['name']) : '',
    '#description' => t("Enter human name for this newsletter. Name's length is limited to 128 characters."),
    '#required' => TRUE,
  );
  
  $form['enabled'] = array(
    '#type' => 'radios',
    '#title' => t('Enable this sequenced newsletter'),
    '#default_value' => isset($old_data['enabled']) ? check_plain($old_data['enabled']) : 0,
    '#description' => t("When enabled sequenced newsletters will be sent. When diabled nothing will happen."),
    '#options' => array(t('Disabled'), t('Enabled')),
  );

  $form['tid'] = array(
    '#type' => 'select',
    '#title' => t('Newsletter'),
    '#default_value' => isset($old_data['tid']) ? $old_data['tid'] : 0,
    '#options' => _sequenced_newsletter_get_tids(),
    '#description' => t("Sequenced newsletter. Subscribers will get sequenced emails from this newsletter term."),
    '#required' => TRUE,
  );

  $form['start_time'] = array(
    '#type' => 'date_select',
    '#title' => t('Sequence start'),
    '#date_type'       => DATE_DATETIME,
    '#default_value'   => isset($old_data['start_time']) ? date('Y-m-d H:i', $old_data['start_time']) : date('Y-m-d H:i'),
    '#date_format'     => 'm-d-Y - H:i',
    '#date_timezone'   => date_default_timezone_name(),
    '#description' => t('Time and day when emails will be moved into spool/sending will start.'),
    '#required' => TRUE
  );

  if ($id != -1) {
    $form['reset'] = array(
      '#type' => 'checkbox',
      '#title' => t('Reset time'),
      '#default_value' => 0,
      '#description' => t("Check if you want to reset timing of this newsletter. If you changed Sequence start time, than this is probbably a good idea.<br />WARNING: Mail will be sent immediatley if timer is reseted and send time is set to past."),
    );
  }
  
  $form['interval'] = array(
    '#type' => 'select',
    '#title' => t('Sending interval'),
    '#default_value' => isset($old_data['interval']) ? $old_data['interval'] : 0,
    '#options' => _sequenced_newsletter_get_intervals(),
    '#description' => t("Interval to send sequenced newspetters."),
    '#required' => TRUE,
  );
  
  $form['sort'] = array(
    '#type' => 'select',
    '#title' => t('Sorting field'),
    '#default_value' => isset($old_data['sort']) ? $old_data['sort'] : 0,
    '#options' => _sequenced_newsletter_get_sort(),
    '#description' => t("Which field to use for ordering newsletter issues."),
    '#required' => TRUE,
  );
  
  $form['sqnid'] = array(
    '#type' => 'hidden',
    '#value' => $id,
  );
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  $form['cancel'] = array(
    '#type' => 'button',
    '#value' => t('Cancel'),
    '#name' => 'cancel',
  );

  return $form;
}

function sequenced_newsletter_admin_edit_submit(&$form, &$form_state) {
  // TODO: Check if time conversions acts OK in timezone context.
  $date = strptime($form_state['values']['start_time'], '%Y-%m-%d %H:%M:%S');
  $date = mktime($date['tm_hour'], $date['tm_min'], $date['tm_sec'], $date['tm_mon']+1, $date['tm_mday'], $date['tm_year']+1900);
  
  // Prepare data for DB.
  $insert = array(
    'name' => $form_state['values']['name'],
    'tid' => $form_state['values']['tid'],
    'start_date' => $date,
    'send_interval' => $form_state['values']['interval'],
    'status' => $form_state['values']['enabled'],
    'sort' => $form_state['values']['sort'],
  );

  // Write to DB.
  $db_response = '';
  if ($form_state['values']['sqnid'] == -1) {
    $db_response = drupal_write_record('sequenced_newsletter_conf', $insert);
  }
  else {
    // Reset last send if desired to do so.
    if ($form_state['values']['reset']) {
      $insert['last_send'] = 0;
      drupal_set_message(t("Sequenced newsletter <em>@name</em> was successfully reseted.", array('@name' => $form_state['values']['name'])));
    }
    $insert['sqnid'] = $form_state['values']['sqnid'];
    $db_response = drupal_write_record('sequenced_newsletter_conf', $insert, array( 'sqnid'));
  }

  // Verbose message.
  if ($db_response) {
    drupal_set_message(
      t("Sequenced newsletter <em>@name</em> was successfully saved.", array('@name' => $form_state['values']['name']))
    );
  }
  else {
    drupal_set_message(
      t("An error occured while trying to save <em>@name</em>.", array('@name' => $form_state['values']['name'])),
     	'error'
    );
  }
  
  // Redirect back to basic configuration.
  $form_state['redirect'] = 'admin/settings/sequenced_newsletter';
}

/**
 * Delete callback.
 */
function sequenced_newsletter_admin_delete(&$form_state, $id) {
  $result = db_query("SELECT name FROM {sequenced_newsletter_conf} WHERE sqnid = %d", $id);
  $result = db_fetch_array($result);
  
  $form['sqnid'] = array(
    '#type' => 'value',
    '#value' => $id,
  );

  return confirm_form($form,
    t('Are you sure you want to delete %title?', array('%title' => $result['name'])),
    isset($_GET['destination']) ? $_GET['destination'] : 'admin/settings/sequenced_newsletter',
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

function sequenced_newsletter_admin_delete_submit(&$form, &$form_state) {
  if ($form_state['values']['confirm']) {
    db_query("UPDATE {sequenced_newsletter_conf} SET status=2 WHERE sqnid=%d", $form_state['values']['sqnid']);
  }

  $form_state['redirect'] = 'admin/settings/sequenced_newsletter';
}


/**
 * Returns data about terms in Simplenews vocabulary.
 * 
 * @return array that can be used as options for select form element.
 */
function _sequenced_newsletter_get_tids() {
  //get simplenews terms
  $vid = variable_get('simplenews_vid', -1);
  $terms = db_query("SELECT * FROM {term_data} WHERE vid=%d", $vid);
  $terms_array = Array();
  while ($row = db_fetch_array($terms)) {
    $terms_array[$row['tid']] = t($row['name']);
  }
  
  return $terms_array;
}

/**
 * Returns info about possible interval settings.
 * 
 * @return Array of possible intervals.
 */
function _sequenced_newsletter_get_intervals() {
  return Array(
      0 => t('Daily'),
      1 => t('Weekly'),
      2 => t('Monthly'),
      3 => t('5 minutes'),
      4 => t('1 minute'),
    );
}

/**
 * Returns info about possible ordering settings.
 * 
 * @return Array of possible order fields.
 */
function _sequenced_newsletter_get_sort() {
  return Array(
      0 => t('nid'),
      1 => t('updated time'),
    );
}

/**
 * Loads one newsletter from DB and returns it as array.
 * 
 * @param $id ID of seq. newsletter to be loaded from DB.
 * 
 * @return seq. newsletter as array.
 */
function _sequenced_newsletter_get_item($id) {
  $result = db_query("SELECT * FROM {sequenced_newsletter_conf} WHERE sqnid = %d AND status <> 2", $id);
  if ($result) {
    $item = db_fetch_object($result);
    return array(
      'name' => $item->name,
      'enabled' => $item->status,
      'tid' => $item->tid,
      'start_time' => $item->start_date,
      'interval' => $item->send_interval,
      'sqnid' => $item->sqnid,
      'sort' => $item->sort,
    );
  }
  else {
    drupal_goto('admin/settings/sequenced_newsletter');
  }
}

/**
 * Returns all acitve items as an array.
 * 
 * @return Array that can be passed to theme_table().
 */
function _sequenced_newsletter_get_active_items() {
  $data = array();
  $result = db_query("SELECT *  FROM {sequenced_newsletter_conf} WHERE status <> 2 ORDER BY sqnid ASC");
  
  while ($item = db_fetch_array($result)) {
    $edit = l(t('Edit'), 'admin/settings/sequenced_newsletter/edit/' . $item['sqnid']);
    $delete = l(t('Delete'), 'admin/settings/sequenced_newsletter/delete/' . $item['sqnid']);
    
    $data[] = array(
      check_plain($item['name']),
      $edit . ' ' . $delete, 
    );
  }
  
  return $data;
}
