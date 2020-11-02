<?php

$planetfm_db = array(
    
    'database' => 'PlanetEnterprise',
    'username' => 'FDSADMIN',
    'password' => 'FDSADM1N',
    'host' => 'facilities',
    'driver' => 'sqlsrv',
    'port' => '1433',
    
);

function rbge_cal_drush_command() {
    
    $items = array();

    $items['rbge_cal_planetfm'] = array(
         'description' => "Syncs all upcoming events from planetfm (unless record option is specified)",
         'options' => array(
           'record' => 'The id of a single record to be synced',
         )
    );
    
    return $items;

}

function drush_rbge_cal_planetfm(){
    
    drush_print("Kicking off PlanetFM sync process");
    
    $record_id = drush_get_option('record', false);
    $upcoming = get_upcoming_planetfm($record_id);
    
    drush_print("Recovered " . count($upcoming) . " bookings from PlanetFM");
    
    $count = 0;
    $total = count($upcoming);
    foreach($upcoming as $booking){
        sync_planetfm_booking($booking);
        $count++;
        $percent = round(($count/$total) * 100);
        drush_print( "$count of $total = $percent%");
    }
    
}

function sync_planetfm_booking($booking){
    
    // is this an initial booking or a repeat booking?
    if($booking['bkgIsRepeated_bool'] == 'T' && $booking['bkgRepeatBkgRef'] != $booking['bkgID_ai']){
        sync_planetfm_repeat_booking($booking);
    }else{
        sync_planetfm_initial_booking($booking);
    }
}

function sync_planetfm_initial_booking($booking){
    
    global $user;
    
    drush_print($booking['bkgID_ai'] . " is INITIAL" . " - " . $booking['bkgDesc']);
    
    // is there an associated node?
    $query = new EntityFieldQuery();
    $result = $query->entityCondition('entity_type', 'node')
      ->entityCondition('bundle', 'event')
      ->fieldCondition('field_planetfm_id', 'value', $booking['bkgID_ai'] , '=');
    $result = $query->execute();

    // we don't find the event
    if(!isset($result['node']) || count($result['node']) == 0 ){
        
        drush_print('   No event found for planetfm id: ' . $booking['bkgID_ai']);
        
        // create a new event for the planetfm booking object
        $values = array(
          'type' => 'event',
          'uid' => $user->uid,
          'status' => 1,
          'comment' => 1,
          'promote' => 0,
        );
        
        $entity = entity_create('node', $values);
        $event = entity_metadata_wrapper('node', $entity);
        
        // set minimal data
        $event->title = iconv('Windows-1252', 'UTF-8', $booking['bkgDesc']);
        $event->field_planetfm_id = $booking['bkgID_ai'];
        $event->field_event_category = 10;
        
        // save it.
        $event->save();
        
        drush_print('   Created event id: ' . $event->getIdentifier());
    
    }else{
        
        // we have a event so load it
        $event_ids = array_keys($result['node']);
        $events = entity_load('node', $event_ids);
        $event = entity_metadata_wrapper('node', array_pop($events));
        
        drush_print('   Event found for planetfm id: ' . $booking['bkgID_ai'] . ' with id ' . $event->getIdentifier());
        
        // do nothing if this event has been edited in UC
        if( has_been_edited_in_uc($event)) return;
        
    }
    
    // set the values in the event itsef
    $event->field_event_venue = get_venue_id_for_room_id($booking['rmID']);
    
    // bung a note in
    $note = "Imported from PlanetFM\nCreated by: ";
    if($booking['bkgReqByFirstName']) $note .= $booking['bkgReqByFirstName'] . " ";
    if($booking['bkgReqByLastName']) $note .= $booking['bkgReqByLastName'] . " ";
    if($booking['bkgCreatedDate']) $note .= "\nOn: " . $booking['bkgCreatedDate'];
    if($booking['bkgReqByEmail']) $note .= "\nEmail: " . $booking['bkgReqByEmail'];
    if($booking['bkgReqByTelephone']) $note .= "\nPhone: " . $booking['bkgReqByTelephone'];

    $event->field_notes_to_staff->set( array(
      "value"  => "<p>$note</p>",
      "format" => "full_html"
    ) );
    
    // set it to exclusive use so that it blocks other events
    $event->field_exclusive_use_of_venue->set(1);
    
    // avoid notifying managers
    $event->field_venue_approval_requested->set(0);
    $event->field_cat_approval_requested->set(0);

    // we remove all the repeat dates because they may have been changed and will come back in a minute!
    foreach($event->field_event_date->getIterator() as $delta => $term_wrapper){
        drush_print("removing:" . $delta);
        $event->field_event_date[$delta]->set(null);
    }
    
    // update it with any changes
    check_repeat_date($event, $booking);
    
    $event->save();
        
}

function sync_planetfm_repeat_booking($booking){
    
    drush_print($booking['bkgID_ai'] . " is REPEAT of " . $booking['bkgRepeatBkgRef'] . " - " . $booking['bkgDesc']);
        
    // is there an associated node for initial event this is a repeat of.
    $query = new EntityFieldQuery();
    $result = $query->entityCondition('entity_type', 'node')
      ->entityCondition('bundle', 'event')
      ->fieldCondition('field_planetfm_id', 'value', $booking['bkgRepeatBkgRef'] , '=');
    $result = $query->execute();

    // we don't find the event - get out of here
    if(!isset($result['node']) || count($result['node']) == 0 ){
        drush_print("    Not found initial event for booking so ignoring it." );
        return;
    }
    
    // got a node so wrap it up as an event
    $event_ids = array_keys($result['node']);
    $events = entity_load('node', $event_ids);
    $event = entity_metadata_wrapper('node', array_pop($events));
    
    // do nothing if this event has been edited in UC
    if( has_been_edited_in_uc($event)) return;
    
    // send it off to add this as a repeat booking
    check_repeat_date($event, $booking);
    
    // if there is but this is a cancel then cancel it.
    // fixme
    
    $event->save();
}

function has_been_edited_in_uc($event){
    
    // if there is something in the event_date repeat rule
    $dateValue = $event->field_event_date->value();
    
    // if there is no date then we assume we are creating it
    if(!$dateValue || count($dateValue) == 0) return false;
    
    // we are only interested in the first one as it will have the repeat rule
    if(isset($dateValue[0]['rrule']) &&  strlen($dateValue[0]['rrule']) > 0){
        drush_print('    Repeat rule edited in UC so ignoring');
        return true;
    }
    
    if(is_array($event->body->value())){
        drush_print('    Description edited in UC so ignoring');
        return true; 
    }
    
    drush_print('    No sign or edit in UC so continuing');
    return false;
    
}

/*
 * make sure there is a repeat for this record if it has one...
 */
function check_repeat_date($event, $booking){
   
   //var_dump($booking['BkgDate']);
   $start_time = new DateTime($booking['BkgDate']);
   $newformat = $start_time->format(DATE_ATOM);
   //var_dump($newformat);
      
   $date_string = substr($start_time->format(DATE_ATOM), 0,10);
   
   list($start_hour, $hour_part) = explode('.', $booking['bkgStart'], 2); // split the decimal hours e.g. 21.5
   $start_mins = round(floatval('0.' . $hour_part) * 60); // work out the minutes
   $start_time = $date_string . 'T' . $start_hour . ':' . $start_mins . ':00';

   // change it to utc
   $start_dt = new DateTime($start_time,  new DateTimeZone('Europe/London'));
   $start_dt->setTimeZone(new DateTimeZone('UTC'));
   $start_time = $start_dt->format('Y-m-d\TH:i:s');

   list($end_hour, $hour_part) = explode('.', $booking['bkgEnd'], 2); // split the decimal hours e.g. 21.5
   $end_mins = round(floatval('0.' . $hour_part) * 60); // work out the minutes
   $end_time = $date_string . 'T' . $end_hour . ':' . $end_mins . ':00';
   
   // change it to utc
   $end_dt = new DateTime($end_time,  new DateTimeZone('Europe/London'));
   $end_dt->setTimeZone(new DateTimeZone('UTC'));
   $end_time = $end_dt->format('Y-m-d\TH:i:s');
   
   // does this repetition exist? If so don't add it again.
   // this happens in events that have been created from initial bookings
   // that are now in the past so have not been scrubbed down
   for ($i=0; $i < count($event->field_event_date); $i++) { 
        $val = $event->field_event_date[$i]->value();
        if($val['value'] == $start_time && $val['value2'] == $end_time){
            
            if($booking['bkgCancelledReason'] == null){
                drush_print("   repeat date exists - skipping");
            }else{
                drush_print("   repeat date exists but is cancelled - deleting");
                $val = $event->field_event_date[$i]->set(null);
            }
            
            return;
        }
   }
   
   
   if($booking['bkgCancelledReason'] != null){
       //var_dump($booking['bkgCancelledReason']);
       drush_print("   repeat date does not exist but is cancelled - skipping");
       return;
   }
   
   // this could be the wrong way of doing it. Better to add it as a repeat rule?
   
   // set the booking date as a repetition
   $event->field_event_date[] = array(
          'all_day' => false,
          'value' => $start_time,
          'value2' => $end_time,
          //'timezone_db' => 'Europe/London',
          //'date_time' => 'date',
          //'timezone' => 'Europe/London',
          //'show_repeat_settings' => 1,
          //'rrule' => null,
          //'show_todate' => 1,
        );
   
   $event->save(); 
   drush_print("   repeat added to " . $event->getIdentifier());
   

}

function get_venue_id_for_room_id($room_id){
    
    $venue_tid = 1;
    
    switch ($room_id) {

        case 5: // lecture theatre
            $venue_tid = 49;
            break;
        
        case 6: // conference room
            $venue_tid = 50;
            break;

        case 7: // boardroom annex
            $venue_tid = 52;
            break;

        case 8: // teaching lab
            $venue_tid = 54;
            break;
                
        case 9: // lecture room 1
            $venue_tid = 53;
            break;

        case 10: // board room
            $venue_tid = 51;
            break;

        case 14: // fletcher building
            $venue_tid = 57;
            break;

        case 15: // caledonian hall
            $venue_tid = 70;
            break;

        case 16: // lecture room 2
            $venue_tid = 56;
            break;

        case 17: // ICT Suite
            $venue_tid = 55;
            break;

        case 18: // INV house basement gallery
            $venue_tid = 36;
            break;
            
        case 19: // nursery potting shed
            $venue_tid = 71;
            break;

        case 21: // john muir grove
            $venue_tid = 73;
            break;

        case 22: // MARQUEE SITE (Inv House)
            $venue_tid =72;
            break;

        case 23: // PATRICK GEDDES ROOM
            $venue_tid = 22;
            break;

        case 24: // REAL LIFE SCIENCE STUDIO
            $venue_tid = 21;
            break;

        case 25: // GATEWAY MEETING ROOM
            $venue_tid = 24;
            break;

        case 37: // DAVID DOUGLAS ROOM
            $venue_tid = 23;
            break;

        case 38: // JOHN HOPE FUNCTION SUITE
            $venue_tid = 20;
            break;
        
        default:
            $venue_tid = 1;
            break;
    }
    
    return $venue_tid;
}

function get_upcoming_planetfm($record_id){
    
    $upcoming = array();
    
    $link = mssql_connect('10.0.20.33','FDSADMIN','FDSADM1N');

    if (!$link) {
       die('Something went wrong while connecting to MSSQL');
    }else{
       drush_print('nice things');
    }

    if (!mssql_select_db('PlanetEnterprise', $link))
       die('Unable to select database!');
       
    // work on a year before today
    $start = new DateTime();
    $start->sub(new DateInterval('P1Y'));
    $start = $start->format('Y-m-d');
    
    $end = ('2019-01-01');
    
    drush_print("All bookings with initial booking after $start");

    $sql = "select rmID, rmDescription, FDS.Bookings.* from FDS.Bookings join FDS.Rooms on FDS.Bookings.rmID = FDS.Rooms.rmID_ai where BkgDate > '$start' and BkgDate < '2019-01-01' and BkgStatus != 'P'";

    if($record_id){
        $sql .= " and (FDS.Bookings.bkgRepeatBkgRef = $record_id OR FDS.Bookings.BkgRef = $record_id)";
    }

    // do it in order
    $sql .= " order by bkgID_ai";
    
    drush_print($sql);

    $result = mssql_query($sql, $link);

    while ($row = mssql_fetch_array($result)) {
      $upcoming[]= $row;
    }

    mssql_free_result($result);

    mssql_close($link);
    
    return $upcoming;
    
}
