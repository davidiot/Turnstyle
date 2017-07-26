package com.example.android.bluetoothlegatt;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

public class SettingsActivity extends Activity {
    private EditText populationThreshold;
    private Button saveChanges;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        populationThreshold = (EditText) findViewById(R.id.populationThreshold);
        saveChanges = (Button) findViewById(R.id.saveChanges);
        String previousPopulationThreshold = getIntent().getStringExtra("Population Threshold");
        if (previousPopulationThreshold != null) {
            populationThreshold.setHint(previousPopulationThreshold);
        } else {
            populationThreshold.setHint("100");
        }
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_settings);
        saveChanges.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String newPopulationThreshold = populationThreshold.getText().toString().trim();
                Intent i = new Intent(SettingsActivity.this, TurnstyleControlActivity.class);
                i.putExtra("Population Threshold", newPopulationThreshold);
                startActivity(i);
            }
        });
    }
}
