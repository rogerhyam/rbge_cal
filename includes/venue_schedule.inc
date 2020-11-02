<?php


function rbge_cal_venue_schedule($year, $month, $day, $venue_tid){
    
    // we capture the output because we are going
    ob_start("rbge_cal_venue_pdf");
    
    // Set up the dates
    $start_date = DateTime::createFromFormat("Y-m-d", "$year-$month-$day");
    $start_date->setTime(0,0,0); // start at beginning of first day
    $end_date = clone $start_date; // finish at another day
    $end_date->add(new DateInterval('P7D')); // which is seven days later
    $end_date->sub(new DateInterval('PT1S')); // knock a second off or we get all day events on the next day
    $end_date_displayed = clone $end_date; // but display a different end date
    $end_date_displayed->sub(new DateInterval('P1D')); // that is only 6 days later.
    
    // get the tree of events for the time period
    $period = array();
    $period['start'] = $start_date->format(DATE_ATOM);
    $period['start_timestamp'] = $start_date->getTimestamp();
    $period['end'] = $end_date->format(DATE_ATOM);
    $period['end_timestamp'] = $end_date->getTimestamp();
    $period['duration'] =  $period['end_timestamp']  - $period['start_timestamp'];
    
    module_load_include('inc', 'rbge_cal', 'includes/venue_event_tree');
    $venue_tree = get_venue_event_tree($period);
    $venue_data = $venue_tree[$venue_tid];
    
    // load the venue
    $venue = taxonomy_term_load($venue_tid);
    
    // set the download file name
    $_REQUEST['rbge-cal-file-name'] = "Week_Schedule_for_venue_" . $venue->tid . "_on_" . $start_date->format('Y-m-d') . '.pdf';;
    
    // start writing it out
    echo "<!DOCTYPE html>";
    echo "<html>";
    echo "<head>";
    echo "<meta charset=\"UTF-8\">";
    echo "</head>";
    echo "<body>";
    
    echo "<h1>$venue->name</h1>";
    echo "<hr/>";
    
    for($i = 0; $i < 7; $i++){
        rbge_cal_write_week_day($i, $venue_data, $start_date);
    }
    
    echo "\n<hr/>";
    
    $now = new DateTime();
    $now_str = $now->format('l jS F @ H:i');
    echo "<p>This schedule was printed on $now_str. Bookings may have changed since then.";
    
    echo "</body>";
    echo "</html>";
    
    ob_end_flush();
    drupal_exit();
    
}

function rbge_cal_write_week_day($i, $venue_data, $start_date){
    
    $day_date = clone $start_date;
    $day_date->add(new DateInterval('P'. $i .'D'));
    $day_data_str = substr($day_date->format(DATE_ATOM), 0,10); // used to spot the events on this day
    
    echo "\n<h2>";
    echo $day_date->format("l jS F");
    echo "</h2>\n";
    
    $todays_events = array();
    if(isset($venue_data->events)){
        foreach($venue_data->events as $event_data){
            $event_repeat_start = $event_data[1]->start;
            if(preg_match('/^'. $day_data_str .'/', $event_repeat_start)){
                $todays_events[] = $event_data;
            }
        }    
    }
    
    if(count($todays_events)){
        $first = true;
        foreach($todays_events as $event_data){
            if(!$first) echo "<br/>\n";
            $e_start = new DateTime($event_data[1]->start);
            echo $e_start->format('H:i');
            echo '-';
            $e_end = new DateTime($event_data[1]->end);
            echo $e_end->format('H:i');
            echo "&nbsp;";
            echo $event_data[0]->title;
            $first = false;
        }
    }else{
        echo "<p>Nothing Scheduled</p>";
    }
    
    
}

function rbge_cal_venue_pdf($buffer){


    // write the buffer to a temporary file
    $temp_html = tempnam(sys_get_temp_dir(), 'rbge-cal-schedule-html-');
    $temp_html .= '.html'; // needs an html ending
    file_put_contents($temp_html, $buffer);
    
    // make another temporary file with the pdf in it
    $temp_pdf = tempnam(sys_get_temp_dir(), 'rbge-cal-schedule-pdf-');
    $temp_pdf .= '.pdf'; // just to be safe
    $cmd = "xvfb-run --server-args=\"-screen 0, 1024x768x24\" wkhtmltopdf $temp_html $temp_pdf";
    $t = shell_exec($cmd);
    
    // return the pdf
    $download_name =  $_REQUEST['rbge-cal-file-name']; //"schedule.pdf";
    header("Content-type:application/pdf");
    header("Content-Disposition:attachment;filename=$download_name");
    
    $out = file_get_contents($temp_pdf);
    
    // clean up the files
    unlink($temp_html);
    unlink($temp_pdf);
    
    return $out;
    
}

